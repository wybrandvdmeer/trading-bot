#include <iostream>
#include <chrono>
#include <thread>
#include <ctime>

#include "candle.h"
#include "position.h"
#include "yahoo_api.h"
#include "db_api.h"
#include "indicators.h"
#include "logger.h"
#include "alpaca_api.h"
#include "tradingbot.h"

using namespace std;

#define SELL_NEGATIVE_TREND_LENGTH 4
#define BUY_POSITIVE_TREND_LENGTH 2

#define SMA_50_200_POSITIVE_TREND_LENGTH 3

#define CANDLE_RANGE 	"2d"
#define CANDLE_INTERVAL "1m"

// Setting of 0.5 causes to catch only the main trade. 
// When 0 the buying selling is only done whem macd > signal and positive/negative trend.
// Alpaca charges low commision.
#define RELATIVE_HIST 0.1

// Om ons te beschermen tegen pre-market buying en dan bij opening Selling scenario's.
#define OPENINGS_WINDOW_IN_MIN 10
#define QUALITY_CANDLES 0.9

// Only buy when price has exceed sma + sma * SMA_RELATIVE_DISTANCE
#define SMA_RELATIVE_DISTANCE 0.05

#define MAX_LAG_TIME 90

/* 
Gap & Go: identificeer een hogere opening tov de vorige dag en lift dan mee na bijv de 1e pull back.
Trendfollowing: Identificeer op dag basis een trend en stap dan in aan het begin vd trend op basis
van bijv de macd.

histogram: 3x dalen (eventueel 2x) dan verkopen, of ihgv signal cross-over.

selectie uit top 5 topGainers.

beurs nse begint om 15.30 localtime -> 9.30 EDT
*/

tradingbot::tradingbot() {
	tradingbot::force = false;
	tradingbot::debug = false;
	tradingbot::disable_alpaca = false;
	tradingbot::macd_set_point = 0;
	tradingbot::time_of_prv_candle = 0;
	tradingbot::strategy = "macd";
	tradingbot::finished_for_the_day = false;
	tradingbot::slave = false;
}

void tradingbot::trade(int top_gainers_idx) {
	tradingbot::ticker = ticker;
	vector<std::string> * top_gainers=NULL;
	bool schema_created = false;

	if(debug) {
		yahoo.debug = true;
		db.debug = true;
	}

	if(slave) {
		tg.slave = true;
	}

	db.strategy = strategy;

	/* Back-testing against a db file. 
	*/
	if(!db_file.empty()) {
		disable_alpaca = true;
		std::string base_name = db_file.substr(db_file.find_last_of("/\\") + 1);
		ticker = base_name.substr(0, base_name.find("-"));
		db.ticker = ticker;
		db.drop_db();
		db.create_schema();

		std::vector<candle*> *candles = db.get_candles(db_file);

		int day_break_position = find_position_of_last_day(candles);
		if(day_break_position == -1) {
			log.log("Cannot determine position of last day.");
			return;
		}

		time_of_prv_candle = candles->at(day_break_position - 1)->time;

		for(int idx=day_break_position + 1; idx < candles->size(); idx++) {
			std::vector<candle*> * v = new std::vector<candle*>();
			for(int idx2=0; idx2 <= idx; idx2++) {
				v->push_back(candles->at(idx2));
			}

			trade(v);
		}

		std::vector<position*> * positions = db.get_closed_positions();
		int no_of_stocks=0;
		float gain=0;
		for(auto p : *positions) {
			no_of_stocks += p->no_of_stocks;
			gain += ((p->sell_price - p->stock_price) * p->no_of_stocks);
		}
		log.log("Number of trades: %d\nNumber of stocks: %d\nGain: %f",
			positions->size(),
			no_of_stocks, 
			gain);

		return;
	}

	while(true) {
		if(!tradingbot::force && !nse_is_open()) {
			if(!ticker.empty()) {
				ticker.erase();
			}
			db.reset();
			black_listed_tickers.clear();
			finished_for_the_day = false;
			std::this_thread::sleep_for(std::chrono::milliseconds(5 * 1000)); 
			continue;
		}

		if(tradingbot::ticker.empty()) {
			top_gainers = tg.get();
			int tg_idx = get_top_gainer(top_gainers, black_listed_tickers, top_gainers_idx);
			if(tg_idx == -1) { 
				log.log("No top gainers.");
			} else {
				tradingbot::ticker = top_gainers->at(tg_idx);
				schema_created = false;
			}
		}

		int sleep = 60;
	
		if(!ticker.empty()) {
			db.ticker = tradingbot::ticker;
			std::vector<candle*> * candles = yahoo.stockPrices(ticker, CANDLE_INTERVAL, CANDLE_RANGE);
			if(candles != NULL) {
				if(!get_quality_candles(candles)) {
					if(schema_created && db.get_open_position(ticker) != NULL) {
						log.log("\nQ of candles too low, but thrs an open position for ticker (%s).", 
							ticker.c_str());
					} else {
						log.log("\nQ of candles too low, ticker (%s) is blacklisted.", ticker.c_str());
						tradingbot::black_listed_tickers.push_back(ticker);
						tradingbot::ticker.erase();
						sleep = 1;
					}
				} else {
					if(!schema_created) {	
						db.create_schema();
						schema_created = true;
					}

					time_of_prv_candle = db.select_max_candle_time();
					if(time_of_prv_candle == 0) {
						time_of_prv_candle = get_gmt_midnight();
					}

					trade(candles);
				}
			} else {
				log.log("No candles received");
			}
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(sleep * 1000));
	}
}

bool candle_cmp(candle * c1, candle * c2) { 
	return c1->time < c2->time;
}

void tradingbot::trade(std::vector<candle*> *candles) {
	/* Delete non valid candles. 
	*/	
	for(std::vector<candle*>::iterator it = candles->begin(); it != candles->end();) {
		if((*it)->is_valid()) {
			it++;
		} else {
			candles->erase(it);
		}
	}

	std::sort(candles->begin(), candles->end(), candle_cmp);

	std::vector<float> close_prices;
	for(auto c : *candles) {
		close_prices.push_back(c->close);
	}

	ind.reset(ticker);
	ind.calculate_sma_50(close_prices);
	ind.calculate_sma_200(close_prices);
	ind.calculate_macd(close_prices);
	max_delta_close_sma_200 = db.select_max_delta_close_sma_200() * SMA_RELATIVE_DISTANCE;

	ind.custom_ind[0] = NULL;
	ind.custom_ind[1] = NULL;
	ind.custom_ind[2] = NULL;

	/* It can be that Yahoo sends 2 new candles in the same batch. We calculated all the indicators
	on these 2 new candles, but we want to trade only on the most recent candle.
	Dont trade on old data.
	*/
	candle * latest = candles->at(candles->size() - 1);

	log.log("Ticker: %s, time latest candle: %s", 
		ticker.c_str(), date_to_string(latest->time).c_str());
	if(latest->time > time_of_prv_candle) {
		log.log("id: %d, (%ld -> %s) - open/close: (%.5f,%.5f), sma50: %.5f, sma200: %.5f, sma200-close-delta: %.5f, macd: %.5f, signal: %.5f, histogram: %.5f", 
			latest->id,
			latest->time,
			date_to_time_string(latest->time).c_str(),
			latest->open, 
			latest->close,
			ind.get_sma_50(0),
			ind.get_sma_200(0),
			max_delta_close_sma_200,
			ind.m.get_macd(0),
			ind.m.get_signal(0),
			ind.m.get_histogram(0));

		if(strategy == "macd") {
			finished_for_the_day = macd_scavenging_strategy(candles, latest);
		} else
		if(strategy == "sma") {
			finished_for_the_day = sma_crossover_strategy(candles, latest);
		} else {
			log.log("Unknown strategy.");
			exit(1);
		}
	
		time_of_prv_candle = latest->time;
	}

	finish(candles);

	log.log("\n");
}

bool tradingbot::sma_crossover_strategy(std::vector<candle*> *candles, candle *candle) {
	float open_0 = candle->open;
	float close_0 = candle->close;

	if(finished_for_the_day) {
		return true;
	}

	bool finished_for_the_day = candle_in_nse_closing_window(candle);
	
	position * p = db.get_open_position(ticker);
	if(p != NULL) {
		bool bSell = false;
		if(ind.get_sma_50(0) - ind.get_sma_200(0) <= 0) { 
			log.log("sell: sma_50 is below sma_200.");
			bSell = true;
		} else
		if(finished_for_the_day) {
			log.log("sell: current candle is in closing window. Trading day is finished.");
			bSell = true;
		} else
		if(close_0 >= p->sell_off_price) {
			p->stop_loss_activated = 0;
			log.log("sell: current price (%f) is above selling price (%f)." ,
				close_0 , p->sell_off_price);
			bSell = true;
		} else
		if(close_0 <= p->loss_limit_price) {
			p->stop_loss_activated = 1;
			log.log("sell: current price (%f) is below stop-loss price (%f)." ,
				close_0 , p->loss_limit_price);
			bSell = true;
		}

		if(bSell) {
			p->sell_price = close_0;

			/* In case of back-testing, the candle time is leading. 
			*/
			if(db_file.empty()) {
				p->sell = time(0);
			} else {
				p->sell = candle->time;
			}
			sell(p);

			/* One shot per day. 
			*/	
			finished_for_the_day = true;
		}

		return finished_for_the_day;
	}

	/* Buy logic. 
	*/
	if(in_openings_window(candle->time)) { // for back-testing.
 		log.log("no trade: Candle is still in openings window."); 
	} else
	if(!ind.is_sma_50_200_diff_trending(SMA_50_200_POSITIVE_TREND_LENGTH, true)) {
		log.log("no trade: not a positive trend on the sma-50/200 indicators.");
	} else 
	if(close_0 < ind.get_sma_200(0) + max_delta_close_sma_200) {
		log.log("no trade: price (%f) is below sma200 (%f + %f).", close_0, ind.get_sma_200(0), 
			max_delta_close_sma_200);
	} else
	if(ind.get_sma_200(0) >= ind.get_sma_50(0)) {
		log.log("no trade: sma_200(%f) is greater then sma_50(%f).",
			ind.get_sma_200(0), 
			ind.get_sma_50(0));
	} else
	if(!in_second_positive_sma_period(candles)) {
		log.log("no trade: not in second positive sma period.");
	} else 
	if(db_file.empty() && time(0) - candle->time >= MAX_LAG_TIME) {
		log.log("no trade: difference (%ld) candle time (%ld) vs current-time (%ld) is too great.",
		time(0) - candle->time,
		candle->time,
		time(0));
	} else {
		buy(ticker, close_0, candle->time);
	}

	return false;
}

bool tradingbot::macd_scavenging_strategy(std::vector<candle*> *candles, candle *candle) {
	float open_0 = candle->open;
	float close_0 = candle->close;

	if(finished_for_the_day) {
		return true;
	}

	bool finished_for_the_day = candle_in_nse_closing_window(candle);
	
	position * p = db.get_open_position(ticker);
	if(p != NULL) {
		bool bSell = false;
		if(ind.m.is_histogram_trending(SELL_NEGATIVE_TREND_LENGTH, false)) {
			log.log("sell: histogram trend is negative.");
			bSell = true;
		} else
		if(finished_for_the_day) {
			log.log("sell: current candle is in closing window. Trading day is finished.");
			bSell = true;
		} else
		if(ind.m.get_macd(0) <= ind.m.get_signal(0)) {
			p->stop_loss_activated = 0;
			log.log("sell: macd (%f) is below signal (%f)." ,
				ind.m.get_macd(0) , ind.m.get_signal(0));
			bSell = true;
		} else
		if(close_0 >= p->sell_off_price) {
			p->stop_loss_activated = 0;
			log.log("sell: current price (%f) is above selling price (%f)." ,
				close_0 , p->sell_off_price);
			bSell = true;
		} else
		if(close_0 <= p->loss_limit_price) {
			p->stop_loss_activated = 1;
			log.log("sell: current price (%f) is below stop-loss price (%f)." ,
				close_0 , p->loss_limit_price);
			bSell = true;
		}

		if(bSell) {
			p->sell_price = close_0;

			/* In case of back-testing, the candle time is leading. 
			*/
			if(db_file.empty()) {
				p->sell = time(0);
			} else {
				p->sell = candle->time;
			}
			sell(p);
		}

		return finished_for_the_day;
	}
	
	macd_set_point = get_macd_set_point(ind.m, candles);

	/* Buy logic. 
	*/
	if(in_openings_window(candle->time)) { // for back-testing.
 		log.log("no trade: Candle is still in openings window."); 
	} else
	if(!ind.m.is_histogram_trending(BUY_POSITIVE_TREND_LENGTH, true)) {
		log.log("no trade: not a positive trend on the macd histogram.");
	} else 
	if(!ind.is_sma_50_200_diff_trending(SMA_50_200_POSITIVE_TREND_LENGTH, true)) {
		log.log("no trade: not a positive trend on the sma-50/200 indicators.");
	} else 
	if(close_0 < ind.get_sma_200(0) + max_delta_close_sma_200) {
		log.log("no trade: price (%f) is below sma200 (%f + %f).", close_0, ind.get_sma_200(0), 
			max_delta_close_sma_200);
	} else
	if(ind.m.get_macd(0) <= ind.m.get_signal(0) + macd_set_point) {
		log.log("no trade: macd(%f) is smaller then signal + set-point (%f + %f).",
			ind.m.get_macd(0), 
			ind.m.get_signal(0),
			macd_set_point);
	} else
	if(db_file.empty() && time(0) - candle->time >= MAX_LAG_TIME) {
		log.log("no trade: difference (%ld) candle time (%ld) vs current-time (%ld) is too great.",
			time(0) - candle->time,
			candle->time,
			time(0));
	} else {
		buy(ticker, close_0, candle->time);
	}

	return false;
}

float tradingbot::get_macd_set_point(macd m, std::vector<candle*> *candles) {
	int start = find_position_of_last_day(candles);
	if(start == -1) {
		log.log("Cannot find position of last day.");
		return 0;
	}

	float max_hist=0;
	for(int idx=start; idx < candles->size(); idx++) {
		int day = candles->size() - idx;
		if(max_hist < m.get_histogram(day)) {
			max_hist = m.get_histogram(day);
		}
	}

	log.log("Max histogram: %f", candles->size(), max_hist);

	return RELATIVE_HIST * max_hist;
}

void tradingbot::finish(std::vector<candle*> * candles) {
	db.insert_candles(ticker, candles, ind);

	/* When backtesting, dont throw away the candles. 
	*/
	if(!db_file.empty()) {
		return;
	}
	
	for(auto c : *candles)  {
		delete c;
	}

	delete candles;

	log.log("Deleted memory.");
}

void tradingbot::buy(std::string ticker, float stock_price, long buy_time) {
	position p;
	p.ticker = ticker;
	p.no_of_stocks = (int)(200.0/stock_price);
	p.stock_price = stock_price;
	p.buy = buy_time;

	if(p.no_of_stocks <= 0) {
		log.log("No-of-stocks calculated is 0.");
		return;
	}

	p.loss_limit_price = stock_price - (stock_price/100) * 2;	

	// 3 percent gain is reasonable.
	p.sell_off_price = stock_price + (stock_price/100) * 3;	

	log.log("Buy (%s), price: %f, number: %d, sell_off_price: %f, loss-limit: %f", 
		ticker.c_str(), 
		p.stock_price,
		p.no_of_stocks,
		p.sell_off_price,
		p.loss_limit_price);

	if(disable_alpaca || alpaca.open_position(p)) {
		db.open_position(p);
	}
}

void tradingbot::sell(position *p) {
	log.log("Sell (%s), price: %f, number: %d, sell_price: %f, loss-limit: %f, stop_loss_activated: %d", 
		ticker.c_str(), 
		p->stock_price,
		p->no_of_stocks,
		p->sell_price,
		p->loss_limit_price,
		p->stop_loss_activated);

	if(disable_alpaca || alpaca.close_position(*p)) {
		db.close_position(*p);
	}
}
	
bool tradingbot::get_quality_candles(std::vector<candle*> *candles) {
	if(candles->size() < 100) {
		log.log("Q of candles too low: not enough (%ld) candles.", candles->size());
		return false;
	}

	int non_valid_candles=0;
	for(auto c : *candles) {
		if(!c->is_valid()) {
			non_valid_candles++;
		}
	}

	float quality = ((float)(candles->size() - non_valid_candles))/candles->size();

	log.log("Q: %f, noOfCandles: %ld, noOfNonValidCandles: %d", 
		quality, candles->size() , non_valid_candles);
	
	return quality >= QUALITY_CANDLES;
}

bool tradingbot::candle_in_nse_closing_window(candle * c) {
	time_t now = time(NULL);
	struct tm *tm_struct = gmtime(&now);

	tm_struct->tm_hour = 19;
	tm_struct->tm_min = 45;
	tm_struct->tm_sec = 0;

	long ts = timegm(tm_struct)%(24 * 3600);
	long ts_candle = (c->time)%(24 * 3600);

	return ts <= ts_candle && ts_candle < ts + 15 * 60;
}

bool tradingbot::nse_is_open() {
	time_t now = time(NULL);
	struct tm *tm_struct = gmtime(&now);

	if(tm_struct->tm_wday == 0 || tm_struct->tm_wday >= 6) {
		return false;
	}

	int hour = tm_struct->tm_hour;
	int minutes = tm_struct->tm_min;

	if(hour >= 13 && hour < 20) {
		if(hour == 13 && minutes <= 30 + OPENINGS_WINDOW_IN_MIN) {
			return false;
		}
		return true;
	}
	return false;
}

std::string tradingbot::date_to_time_string(long ts) {
	ts = ts - 4 * 3600;
	struct tm *t = gmtime(&ts);
	char buf[100];
	sprintf(buf, "%02d:%02d:%02d", 
		t->tm_hour, 
		t->tm_min, 
		t->tm_sec);
	return std::string(buf);
}

bool tradingbot::in_openings_window(long current_time) {
	time_t now = time(NULL);
	struct tm *tm_struct = gmtime(&now);

	tm_struct->tm_hour = 13;
	tm_struct->tm_min = 30;
	tm_struct->tm_sec = 0;

	long ts = timegm(tm_struct)%(24 * 3600);
	current_time = current_time%(24 * 3600);

	return ts <= current_time && current_time < ts + OPENINGS_WINDOW_IN_MIN * 60;
}

int tradingbot::get_gmt_midnight() {
	time_t now = time(NULL);
	struct tm *tm_struct = gmtime(&now);

	tm_struct->tm_hour = 0;
	tm_struct->tm_min = 0;
	tm_struct->tm_sec = 0;
	return timegm(tm_struct);
}

std::string tradingbot::date_to_string(long ts) {
	ts = ts - 4 * 3600;
	struct tm *t = gmtime(&ts);
	char buf[100];
	sprintf(buf, "%d-%02d-%02d %02d:%02d:%02d", 
		t->tm_year + 1900, 
		t->tm_mon + 1, 
		t->tm_mday, 
		t->tm_hour, 
		t->tm_min, 
		t->tm_sec);
	return std::string(buf);
}

void tradingbot::ema_test() {
	std::vector<float> v;
	for(int idx=0; idx < 6; idx++) {
		v.push_back(idx);
	}

	log.log("%f", ind.calculate_ema(2, v));
	exit(0);
}

int tradingbot::get_top_gainer(std::vector<std::string> * top_gainers, 
	std::vector<std::string> black_listed_tickers, int top_gainers_idx) {

	int idx=0;
    for(auto t : *top_gainers) {
		if(idx%3 == top_gainers_idx && 
			std::find(black_listed_tickers.begin(), 
			black_listed_tickers.end(), t) == black_listed_tickers.end()) {
			return idx;
		}
		idx++;
	}

	return -1;
}

int tradingbot::find_position_of_last_day(std::vector<candle*> *candles) {
	int idx=0, day_break_position=-1, first_day, prv_day=-1;
    for(auto c : *candles) {
    	first_day = gmtime(&(c->time))->tm_wday;
        if(prv_day == -1) {
        	prv_day = first_day;
        } else
       	if(prv_day != first_day) {
        	day_break_position = idx;
            prv_day = first_day;
		}
        idx++;
	}
	return day_break_position;
}

bool tradingbot::in_second_positive_sma_period(std::vector<candle*> * candles) {
	int start = find_position_of_last_day(candles);
	if(start == -1) {
		log.log("Cannot find position of last day.");
		return false;
	}
	int period=0;

	bool sma_positive = false;
	for(int idx=start; idx < ind.sma_50.size(); idx++) {
		if(ind.sma_50.at(idx) > ind.sma_200.at(idx)) {
			if(!sma_positive) {
				sma_positive = true;
				period++;
			}
		} else {
			sma_positive = false;
		}
	}

	return period == 2 && sma_positive;
}


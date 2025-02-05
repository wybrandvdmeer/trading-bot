#include <iostream>
#include <chrono>
#include <thread>
#include <ctime>
#include <memory>
#include <filesystem>

#include "candle.h"
#include "position.h"
#include "yahoo_api.h"
#include "db_api.h"
#include "indicators.h"
#include "logger.h"
#include "alpaca_api.h"
#include "tradingbot.h"

using namespace std;

#define CANDLE_RANGE 	"2d"
#define CANDLE_INTERVAL "1m"

// Om ons te beschermen tegen pre-market buying en dan bij opening Selling scenario's.
#define OPENINGS_WINDOW_IN_MIN 10
#define QUALITY_CANDLES 0.9

// Only buy when price has exceed sma + sma * SMA_RELATIVE_DISTANCE
#define SMA_RELATIVE_DISTANCE 0.05

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
	tradingbot::macd_set_point = 0;
	tradingbot::time_of_prv_candle = 0;
	tradingbot::strategy = "macd";
	tradingbot::disable_alpaca = false;
}

void tradingbot::trade() {
	tradingbot::ticker = ticker;

	if(debug) {
		yahoo.debug = true;
		db.debug = true;
	}

	if(strategy == "macd") {
		strat = new macd_scavenging_strategy(&db, &ind);
	} else
	if(strategy == "rmcd") {
		strat = new macd_root_strategy(&db, &ind);
	} else 
	if(strategy == "sma") {
		strat = new sma_crossover_strategy(&db, &ind);
	} else {
		log.log("Unknown strategy.");
		return;
	}

	if(disable_alpaca) {
		strat->disable_alpaca = true;
	}

	log.log("Alpaca key: %s.", std::getenv("APCA_API_KEY_ID"));

	db.strategy = strategy;

	/* Back-testing against a db file. 
	*/
	if(!db_file.empty()) {
		strat->disable_alpaca = true;
		std::string base_name = db_file.substr(db_file.find_last_of("/\\") + 1);
		ticker = base_name.substr(0, base_name.find("-"));
		db.ticker = ticker;
		db.drop_db();
		db.create_schema(id);

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

	if(ticker.empty()) {
		log.log("Check todays datafiles if one is locked by this bot.");
		std::unique_ptr<std::string> ticker_in_progress = get_ticker_from_db();
		if(ticker_in_progress) {
			ticker = *ticker_in_progress;
			has_lock = true;
		}
	}

	while(true) {
		if(!tradingbot::force && !nse_is_open()) {
			/* Reset all vars to begin a new day. 
			*/
			ticker.erase();
			db.reset();
			black_listed_tickers.clear();
			strat->finished_for_the_day = false;
			has_lock = false;

			std::this_thread::sleep_for(std::chrono::milliseconds(5 * 1000)); 
			continue;
		}

		if(tradingbot::ticker.empty()) {
			std::unique_ptr<std::string> u = get_top_gainer(black_listed_tickers);
			if(!u) { 
				log.log("No top gainers.");
			} else {
				ticker = *u;
			}
		}

		int sleep = 60;
	
		if(!ticker.empty()) {
			db.ticker = tradingbot::ticker;
			std::vector<candle*> * candles = yahoo.stockPrices(ticker, CANDLE_INTERVAL, CANDLE_RANGE);
			if(candles != NULL) {
				if(!get_quality_candles(candles)) {
					if(has_lock && db.get_open_position(ticker) != NULL) {
						log.log("\nQ of candles too low, but thrs an open position for ticker (%s).", 
							ticker.c_str());
					} else {
						log.log("\nQ of candles too low, ticker (%s) is blacklisted.", ticker.c_str());
						tradingbot::black_listed_tickers.push_back(ticker);
						tradingbot::ticker.erase();
						has_lock = false;
						sleep = 1;
					}
				} else if(!has_lock && !(has_lock = db.lock_db(tradingbot::id))) {
					log.log("Ticker %s is locked.", ticker.c_str());
					tradingbot::black_listed_tickers.push_back(ticker);
					tradingbot::ticker.erase();
					sleep = 1;
				} else {
					time_of_prv_candle = db.select_max_candle_time();
					if(time_of_prv_candle == 0) {
						time_of_prv_candle = get_gmt_midnight();
					}

					trade(candles);
				}
			} else {
				log.log("No candles received.");
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

	if(latest->time > time_of_prv_candle) {
		log.log("id: %d, (%ld -> %s) - open/close: (%.5f,%.5f), sma50: %.5f, sma200: %.5f, sma200-close-delta: %.5f, macd: %.5f, signal: %.5f, histogram: %.5f", 
			latest->id,
			latest->time,
			latest->get_time_string().c_str(),
			latest->open, 
			latest->close,
			ind.get_sma_50(0),
			ind.get_sma_200(0),
			max_delta_close_sma_200,
			ind.m.get_macd(0),
			ind.m.get_signal(0),
			ind.m.get_histogram(0));

		strat->trade(ticker, candles, latest, max_delta_close_sma_200, !db_file.empty()); 

		time_of_prv_candle = latest->time;
	} else {
		log.log("No trading cause latest candle lies in the past (%s).",
			date_to_string(latest->time).c_str());
	}

	finish(candles);

	log.log("");
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

std::unique_ptr<std::string> 
	tradingbot::get_top_gainer(std::vector<std::string> black_listed_tickers) {
	vector<std::string> * top_gainers = tg.get();
	if(top_gainers == NULL) {
		return std::unique_ptr<std::string>{};
	}

	int idx=0;
    for(auto t : *top_gainers) {
		if(std::find(black_listed_tickers.begin(), black_listed_tickers.end(), t) 
			== black_listed_tickers.end()) {
			std::unique_ptr<std::string> p = std::make_unique<std::string>(t);
			return p;
		}
	}

	return std::unique_ptr<std::string>{};
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

std::string tradingbot::get_sysdate() {
	time_t ts = time(0);
	struct tm *t = localtime(&ts);
	char buf[100];
	sprintf(buf, "%d%02d%02d", 
		t->tm_year + 1900, 
		t->tm_mon + 1, 
		t->tm_mday);
	return std::string(buf);
}

std::unique_ptr<std::string> tradingbot::get_ticker_from_db() {
	std::string sysdate = get_sysdate();
	for(auto entry : filesystem::directory_iterator("/db-files")) {
		std::string db_file = entry.path().string();
		if(db_file.find(sysdate) == std::string::npos) {
			continue;
		}
		if(db_file.find(tradingbot::strategy) == std::string::npos) {
			continue;
		}

		log.log("Inspecting db-file: %s", db_file.c_str());

		int id = db.get_owner_of_db_file(db_file);
		if(id == -1) {
			continue;
		}
		if(id == tradingbot::id) {
			std::filesystem::path p(db_file);
			log.log("Found previous datafile: %s, continuing with it.", p.filename().c_str());
    		
			std::stringstream ss(p.filename());
    		std::string t;
    		std::getline(ss, t, '-');
			return std::make_unique<std::string>(t);
		}
	}
	return std::unique_ptr<std::string>{};
}

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

#define CANDLE_RANGE 	"2d"
#define CANDLE_INTERVAL "2m"
#define MACD_SIGNAL_DIFFERENCE 0.0
#define SMA_200_DIFFERENCE 0.1

/* 
Gap & Go: identificeer een hogere opening tov de vorige dag en lift dan mee na bijv de 1e pull back.
Trendfollowing: Identificeer op dag basis een trend en stap dan in aan het begin vd trend op basis
van bijv de macd.

Top-gainers: queryen op 1m.  later mischien op kwartier.
macd boven 0
macd boven signal
Dan buy in de morgen.
Wanneer cross macd/signal -> sell

selectie uit top 5 topGainers.

beurs nse begint om 15.30 localtime -> 9.30 EDT
*/

tradingbot::tradingbot() {
	tradingbot::sma_200 = 0;
	tradingbot::force = false;
	tradingbot::debug = false;
	tradingbot::disable_alpaca = false;
	tradingbot::prv_macd_signal_diff = -1;
}

/*
macd scalping strategy nog te programmeren:
1> top gainers
2> visualiseren candles
3> risico analyze 
*/

void tradingbot::trade() {
	tradingbot::ticker = ticker;

	if(debug) {
		yahoo.debug = true;
		db.debug = true;
	}

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

		/* Find position of last day.
		*/
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

		for(idx=day_break_position + 1; idx < candles->size(); idx++) {
			std::vector<candle*> * v = new std::vector<candle*>();
			for(int idx2=0; idx2 < idx; idx2++) {
				v->push_back(candles->at(idx2));
			}
			if(trade(v)) {
				break;
			}
		}

		return;
	}

	while(true) {
        if(!tradingbot::force && !nse_forex_is_open()) {
			if(!ticker.empty()) {
				ticker.erase();
			}
            std::this_thread::sleep_for(std::chrono::milliseconds(5 * 1000)); 
            continue;
        }

		bool finished_for_the_day = false;
		if(!ticker.empty()) {
			db.ticker = ticker;
			db.create_schema();

			std::vector<candle*> * candles = yahoo.stockPrices(ticker, CANDLE_INTERVAL, CANDLE_RANGE);
			if(candles != NULL) {
				finished_for_the_day = trade(candles);
			} else {
				log.log("No candles received");
			}
		}

		if(finished_for_the_day) {
			std::this_thread::sleep_for(std::chrono::milliseconds(8 * 60 * 60 * 1000));
		} else {
			std::this_thread::sleep_for(std::chrono::milliseconds(60 * 1000));
		}
	}
}


bool tradingbot::trade(std::vector<candle*> *candles) {
	log.log("\nEvaluating %s", ticker.c_str());

	if(!get_quality_candles(candles)) {
		log.log("\nQuality candles too low.");
		return false;
	}

	candle * current = candles->at(candles->size() - 1);

	/* Skip non valid candle. 
	W'll store it when a valid candle comes along. 
	*/
	if(!current->is_valid()) {
		log.log("Received a non valid candle.");
		return false;
	}

	sma_200 = calc_sma_200(candles);
	macd * m = ind.calculate_macd(candles);

	float open_0 = current->open;
	float close_0 = current->close;

	log.log("open/close: (%f,%f), sma200: %f, macd: %f, signal: %f, ema(20): %f", 
		open_0, 
		close_0,
		sma_200,
		m->get_macd(0),
		m->get_signal(0),
		ind.calculate_ema(20, candles, 0));

	bool finished_for_the_day = 
		candle_in_nse_forex_closing_window(current);
	
	position * p = db.get_open_position(ticker);
	if(p != NULL) {
		bool bSell = false;
		if(prv_macd_signal_diff != -1 && 
			m->get_macd(0) < prv_macd_signal_diff) {
			log.log("sell: prv macd/signal diff (%f) is greater then macd(%f).",
			prv_macd_signal_diff,
			m->get_macd(0));
			bSell = true;
		} else
		if(finished_for_the_day) {
			log.log("sell: current candle is in closing window. Trading day is finished.");
			bSell = true;
		} else
		if(m->get_macd(0) <= m->get_signal(0)) {
			p->stop_loss_activated = 0;
			log.log("sell: macd (%f) is below signal (%f)." ,
				m->get_macd(0) , m->get_signal(0));
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
				p->sell = current->time;
			}
			sell(p);
		}
		
		finish(ticker, candles, m, sma_200);
		return finished_for_the_day;
	}

	if(close_0 < sma_200 + SMA_200_DIFFERENCE) {
		log.log("no trade: price (%f) is below sma200 (%f).", close_0, sma_200 + SMA_200_DIFFERENCE);
	} else
	if(m->get_macd(0) <= m->get_signal(0) + MACD_SIGNAL_DIFFERENCE) {
		log.log("no trade: macd(%f) is smaller then signal (%f).", m->get_macd(0), m->get_signal(0));
	} else {
		/* In case of back-testing, the candle time is leading. 
		*/
		if(db_file.empty()) {
			buy(ticker, close_0, time(0));
		} else {
			buy(ticker, close_0, current->time);
		}
	}

	finish(ticker, candles, m, sma_200);
	return false;
}

void tradingbot::finish(std::string ticker, std::vector<candle*> * candles, macd * m, float sma_200) {
	db.insert_candles(ticker, candles, m, sma_200);
	tradingbot::prv_macd_signal_diff = m->get_macd(0);

	/* When backtesting, dont throw away the candles. 
	*/
	if(!db_file.empty()) {
		return;
	}
	
	for(auto c : *candles)  {
		delete c;
	}

	delete candles;

	delete m;
}

void tradingbot::buy(std::string ticker, float stock_price, long buy_time) {
	position p;
	p.ticker = ticker;
	p.no_of_stocks = 200 / ((int)stock_price);
	p.stock_price = stock_price;
	p.buy = buy_time;

	if(p.no_of_stocks <= 0) {
		log.log("No-of-stocks calculated is 0.");
		return;
	}

	// inzet: 200, risk: 5% -> 20 euro risico.  
	float risk_per_stock = ((float)10)/p.no_of_stocks;
	p.loss_limit_price = stock_price - risk_per_stock;
	p.sell_off_price = stock_price + risk_per_stock * 4;
	
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

float tradingbot::calc_sma_200(std::vector<candle*> * candles) {
	float sma_200 = ind.calculate_sma(200, candles, 0);
	log.log("Calculated sma-200: %f", sma_200);
	return sma_200;
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
	float non_valid_candles=0;
	for(auto c : *candles) {
		if(!c->is_valid()) {
			non_valid_candles++;
		}
	}

	float quality = (candles->size() - non_valid_candles)/candles->size();

	log.log("quality: %f%, noOfCandles: %ld, noOfNonValidCandles: %f", 
		quality, candles->size() , non_valid_candles);
	
	return quality >= 0.9;
}

bool tradingbot::candle_in_nse_forex_closing_window(candle * c) {
	time_t now = time(NULL);
	struct tm *t = gmtime(&now);
	
	if(t->tm_wday != 5) {
		return false;
	}

	t->tm_hour = 20;
	t->tm_min = 50;
	t->tm_sec = 0;

	long ts = timegm(t)%(24 * 3600);
	long ts_candle = (c->time)%(24 * 3600);

	return ts <= ts_candle && ts_candle < ts + 10 * 60;
}

bool tradingbot::nse_forex_is_open() {
	time_t now = time(NULL);
	struct tm *t = gmtime(&now);
		
	if(t->tm_wday == 5 && t->tm_hour > 21) {
		return false;
	}

	if(t->tm_wday == 6) {
		return false;
	}

	if(t->tm_wday == 0 && t->tm_hour < 5) {
		return false;
	}

	return true;
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

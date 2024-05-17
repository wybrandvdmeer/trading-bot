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

beurs nse begint om 15.30
*/

tradingbot::tradingbot() {
	tradingbot::sma_200 = 0;
	tradingbot::force = false;
	tradingbot::debug = false;
	tradingbot::disable_alpaca = false;
}

/*
macd scalping strategy nog te programmeren:
1> top gainers
2> visualiseren candles
3> risico analyze 
*/

void tradingbot::trade(int top_gainers_idx) {
	vector<std::string> * top_gainers=NULL;

	if(debug) {
		yahoo.debug = true;
		db.debug = true;
	}

	/* Back-testing against a db file. 
	*/
	if(!db_file.empty()) {
		std::string base_name = db_file.substr(db_file.find_last_of("/\\") + 1);
		ticker = base_name.substr(0, base_name.find("-"));
		db.ticker = ticker;
		db.drop_db();
		db.create_schema();

		std::vector<candle*> *candles = db.get_candles(db_file);
		trade(candles);
		return;
	}

	while(true) {
        if(!tradingbot::force && !nse_is_open()) {
			if(!ticker.empty()) {
				ticker.erase();
			}
            std::this_thread::sleep_for(std::chrono::milliseconds(5 * 1000)); 
            continue;
        }

        if(tradingbot::ticker.empty()) {
            top_gainers = tg.get();
           
            if(top_gainers->size() == 0) { 
                cout << "No top gainers.";
                exit(1);
            }

			tradingbot::ticker = top_gainers->at(top_gainers_idx);
        }

		if(!ticker.empty()) {
			db.ticker = ticker;
			db.create_schema();
	
			std::vector<candle*> * candles = yahoo.stockPrices(ticker, "1m", "2d");
			if(candles != NULL) {
				trade(candles);
			} else {
				log.log("No candles received");
			}
		}
		
		std::this_thread::sleep_for(std::chrono::milliseconds(60 * 1000));
	}
}


void tradingbot::trade(std::vector<candle*> *candles) {
	log.log("\nEvaluating %s", ticker.c_str());

	if(!get_quality_candles(candles)) {
		log.log("\nQuality candles too low.");
		return;
	}

	sma_200 = calc_sma_200(candles);
	macd * m = ind.calculate_macd(candles);

	candle * current = get_valid_candle(candles, 0);
	candle * previous = get_valid_candle(candles, 1);

	float open_0 = current->open;
	float close_0 = current->close;
	float close_1 = previous->close;

	/* We are only trading if stock is below 20 dollar. 
	*/
	if(close_0 > 20) {
		log.log("Stock price of (%s) is above 20 dollar", ticker.c_str());
		exit(0);
	}

	log.log("open/close: (%f,%f), sma200: %f, macd: %f, signal: %f, ema(20): %f", 
		open_0, 
		close_0,
		sma_200,
		m->get_macd(0),
		m->get_signal(0),
		ind.calculate_ema(20, candles, 0));
	
	position * p = db.get_open_position(ticker);
	if(p != NULL) {
		bool bSell = false;
		if(m->get_macd(0) <= m->get_signal(0)) {
			p->stop_loss_activated = 0;
			log.log("sell: macd (%f) is below signal (%f)." ,
				m->get_macd(0) , m->get_signal(0));
			bSell = true;
		} else
		if(close_0 >= p->sell_price) {
			p->stop_loss_activated = 0;
			log.log("sell: current price (%f) is above selling price (%f)." ,
				close_0 , p->sell_price);
			bSell = true;
		} else
		if(close_0 <= p->loss_limit_price) {
			p->stop_loss_activated = 1;
			log.log("sell: current price (%f) is below stop-loss price (%f)." ,
				close_0 , p->loss_limit_price);
			bSell = true;
		}

		if(bSell) {
			sell(p);
		}

		finish(ticker, candles, m);
		return;
	}

	if(close_0 <= sma_200) {
		log.log("no trade: price (%f) is below sma200 (%f).", close_0, sma_200);
	} else
	if(m->get_macd(0) <= 0) {
		log.log("no trade: macd(%f) is smaller then 0.", m->get_macd(0));
	} else
	if(m->get_macd(0) <= m->get_signal(0)) {
		log.log("no trade: macd(%f) is smaller then signal (%f).", m->get_macd(0), m->get_signal(0));
	} else {
		buy(ticker, close_0);
	}

	finish(ticker, candles, m);
}

void tradingbot::finish(std::string ticker, std::vector<candle*> * candles, macd * m) {
	db.insert_candles(ticker, candles, m);

	for(auto c : *candles)  {
		delete c;
	}

	delete candles;

	delete m;
}

candle * tradingbot::get_valid_candle(std::vector<candle*> * candles, int position) {
	candle *c=NULL;
	int idx = candles->size() - 1 - position;
	while(idx >= 0) {
		c = candles->at(idx);
		if(c->open > 0 && c->close > 0) {
			return c;
		}
		idx--;
	}
	return NULL;
}

void tradingbot::buy(std::string ticker, float stock_price) {

	position p;
	p.ticker = ticker;
	p.buy = time(0);
	p.no_of_stocks = 200 / ((int)stock_price);
	p.stock_price = stock_price;

	if(p.no_of_stocks <= 0) {
		log.log("No-of-stocks calculated is 0.");
		return;
	}

	// inzet: 200, risk: 5% -> 20 euro risico.  
	float risk_per_stock = ((float)10)/p.no_of_stocks;
	p.loss_limit_price = stock_price - risk_per_stock;
	p.sell_price = stock_price + risk_per_stock * 4;
	
	log.log("Buy (%s), price: %f, number: %d, sell_price: %f, loss-limit: %f", 
		ticker.c_str(), 
		p.stock_price,
		p.no_of_stocks,
		p.sell_price,
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
		if(c->open == 0 || c->close == 0) {
			non_valid_candles++;
		}
	}

	float quality = (candles->size() - non_valid_candles)/candles->size();

	log.log("quality: %f%, noOfCandles: %ld, noOfNonValidCandles: %f", 
		quality, candles->size() , non_valid_candles);
	
	return quality >= 0.9;
}

bool tradingbot::nse_is_open() {
	time_t now = time(NULL);
	struct tm *tm_struct = localtime(&now);

	if(tm_struct->tm_wday == 0 || tm_struct->tm_wday >= 6) {
		return false;
	}

	int hour = tm_struct->tm_hour;
	int minutes = tm_struct->tm_min;

	if(hour >= 15 && hour < 22) {
		if(hour == 15 && minutes <= 30) {
			return false;
		}
		return true;
	}
	return false;
}

#include <iostream>
#include <chrono>
#include <thread>
#include <ctime>
#include <memory>
#include <filesystem>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "candle.h"
#include "position.h"
#include "yahoo_api.h"
#include "db_api.h"
#include "indicators.h"
#include "logger.h"
#include "alpaca_api.h"
#include "tradingbot.h"

using namespace std;

#define LOCK_DIR "/tmp/tickers-"

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
}

void tradingbot::trade() {
	tradingbot::ticker = ticker;
	bool schema_created = false;

	if(debug) {
		yahoo.debug = true;
		db.debug = true;
	}

	if(strategy == "macd") {
		strat = new macd_scavenging_strategy(&db, &ind);
	} else
	if(strategy == "dip") {
		strat = new dip_and_rip_strategy(&db, &ind);
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

	db.strategy = strategy;

	/* Back-testing against a db file. 
	*/
	if(!db_file.empty()) {
		strat->disable_alpaca = true;
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
			strat->finished_for_the_day = false;

			std::this_thread::sleep_for(std::chrono::milliseconds(5 * 1000)); 
			continue;
		}

		if(tradingbot::ticker.empty()) {
			std::unique_ptr<std::string> u = get_top_gainer(black_listed_tickers);
			if(!u) { 
				log.log("No top gainers.");
			} else {
				schema_created = false;
				ticker = *u;
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
				} else if(!lock(ticker)) {
					log.log("Ticker (%s) is locked.", ticker.c_str());
					tradingbot::black_listed_tickers.push_back(ticker);
					tradingbot::ticker.erase();
					sleep = 1;
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

		strat->trade(ticker, candles, latest, max_delta_close_sma_200, !db_file.empty()); 

		time_of_prv_candle = latest->time;
	}

	finish(candles);

	log.log("\n");
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

bool tradingbot::lock(std::string ticker) {
	std::string dir = LOCK_DIR + strategy + "-" + get_sysdate();
	std::filesystem::create_directory(dir);
	
	int fd = open((dir + "/" + ticker).c_str(), O_CREAT | O_EXCL, 0644);
	close(fd);
	return fd >= 0;
}

std::string tradingbot::get_sysdate() {
	time_t ts = time(0);
	struct tm *t = localtime(&ts);
	char buf[100];
	sprintf(buf, "%d-%02d-%02d", 
		t->tm_year + 1900, 
		t->tm_mon + 1, 
		t->tm_mday);
	return std::string(buf);
}

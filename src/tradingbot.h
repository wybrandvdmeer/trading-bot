#include <memory>

#include "logger.h"
#include "position.h"
#include "db_api.h"
#include "yahoo_api.h"
#include "indicators.h"
#include "top_gainers.h"
#include "alpaca_api.h"
#include "strategy.h"
#include "sma_crossover_strategy.h"
#include "macd_scavenging_strategy.h"
#include "dip_and_rip_strategy.h"

#ifndef TRADINGBOT_H
#define TRADINGBOT_H

class tradingbot {
public:
	tradingbot();
	std::string ticker, db_file, strategy;
	void trade();
	bool force, debug, disable_alpaca, has_lock=false;
	int id=0;
private:
	class strategy *strat;
	float max_delta_close_sma_200, macd_set_point;
	int time_of_prv_candle;
	top_gainers tg;
	yahoo_api yahoo;
	db_api db;
	indicators ind;
	logger log;
	alpaca_api alpaca;
	std::vector<std::string> black_listed_tickers;
	void trade(std::vector<candle*> * candles);
	bool nse_is_open();
	bool get_quality_candles(std::vector<candle*> *candles);
	void finish(std::vector<candle*> *candles);
	std::string date_to_string(long ts);
	void ema_test();
	int find_position_of_last_day(std::vector<candle*> *candles);
	std::unique_ptr<std::string> get_top_gainer(std::vector<std::string> black_listed_tickers);
	int get_gmt_midnight();
	bool lock(std::string ticker);
	void remove_lock(std::string ticker);
	std::string get_sysdate();
	std::unique_ptr<std::string> get_ticker_from_db();
};

#endif

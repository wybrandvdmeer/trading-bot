#include "logger.h"
#include "position.h"
#include "db_api.h"
#include "yahoo_api.h"
#include "indicators.h"
#include "top_gainers.h"
#include "alpaca_api.h"

#ifndef TRADINGBOT_H
#define TRADINGBOT_H

class tradingbot {
public:
	tradingbot();
	std::string ticker, db_file;
	void trade(int top_gainer_idx);
	bool force, debug, disable_alpaca, slave;
private:
	float max_delta_close_sma_200, macd_set_point;
	int time_of_prv_candle;
	top_gainers tg;
	yahoo_api yahoo;
	db_api db;
	indicators ind;
	logger log;
	alpaca_api alpaca;
	std::vector<std::string> black_listed_tickers;
	bool trade(std::vector<candle*> * candles);
	bool trade_on_candle(std::vector<candle*> *candles, candle * candle);
	bool nse_is_open();
	bool candle_in_nse_closing_window(candle * c);
	void buy(std::string ticker, float stock_price, long buy_time);
	void sell(position * p);
	bool get_quality_candles(std::vector<candle*> *candles);
	void finish(std::vector<candle*> *candles);
	std::string date_to_string(long ts);
	std::string date_to_time_string(long ts);
	void ema_test();
	int find_position_of_last_day(std::vector<candle*> *candles);
	bool in_openings_window(long current_time);
	int get_top_gainer(std::vector<std::string> * top_gainers, 
		std::vector<std::string> black_listed_tickers, int top_gainers_idx);
	float get_macd_set_point(macd m, std::vector<candle*> *candles);
	int get_gmt_midnight();
};

#endif

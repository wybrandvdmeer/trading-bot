#include "logger.h"
#include "position.h"
#include "db_api.h"
#include "yahoo_api.h"
#include "indicators.h"
#include "alpaca_api.h"

#ifndef TRADINGBOT_H
#define TRADINGBOT_H

class tradingbot {
public:
	tradingbot();
	std::string ticker, db_file;
	void trade(std::string ticker);
	bool force, debug, disable_alpaca;
private:
	float sma_200;
	yahoo_api yahoo;
	db_api db;
	indicators ind;
	logger log;
	alpaca_api alpaca;
	bool trade(std::vector<candle*> * candles);
	bool nse_forex_is_open();
	bool candle_in_nse_forex_closing_window(candle * c);
	void buy(std::string ticker, float stock_price, long buy_time);
	void sell(position * p);
	bool get_quality_candles(std::vector<candle*> *candles);
	void finish(std::string ticker, std::vector<candle*> *candles, macd *m, float sma_200);
	float calc_sma_200(std::vector<candle*> *candles);
	std::string date_to_string(long ts);
};

#endif

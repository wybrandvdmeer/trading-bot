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
	std::string ticker;
	tradingbot(std::string ticker);
	int sma_slow_range, sma_fast_range;
	void trade(int offset);
	float calc_sma_200();
	void configure();
private:
	float sma_200;
	yahoo_api yahoo;
	db_api db;
	indicators ind;
	logger log;
	void buy(std::string ticker, float stock_price);
	void sell(position * p);
	candle * get_valid_candle(std::vector<candle*> * candles, int position);
	void log_quality_candles(std::vector<candle*> *candles);
	void finish(std::string ticker, std::vector<candle*> *candles, macd *m);
	alpaca_api alpaca;
};

#endif

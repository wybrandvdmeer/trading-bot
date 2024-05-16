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
	std::string ticker;
	void trade(int offset, int top_gainers_idx);
	void configure();
	bool force, debug;
private:
	float sma_200;
	yahoo_api yahoo;
	db_api db;
	indicators ind;
	logger log;
	alpaca_api alpaca;
	top_gainers tg;
	void trade(int offset);
	bool nse_is_open();
	void buy(std::string ticker, float stock_price);
	void sell(position * p);
	candle * get_valid_candle(std::vector<candle*> * candles, int position);
	bool get_quality_candles(std::vector<candle*> *candles);
	void finish(std::string ticker, std::vector<candle*> *candles, macd *m);
	float calc_sma_200(std::vector<candle*> * candles);
};

#endif

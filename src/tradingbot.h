#include "logger.h"
#include "position.h"
#include "db_api.h"
#include "yahoo_api.h"

#ifndef TRADINGBOT_H
#define TRADINGBOT_H

class tradingbot {
public:
	std::string ticker;
	tradingbot(std::string ticker);
	int smaSlowRange, smaFastRange;
	void trade(int offset);
	void configure();
private:
	yahoo_api yahoo;
	db_api db;
	logger log;
	float calculateSma(int noOfDays, std::vector<candle*> * candles, int offset);
	float calculateEma(int noOfDays, std::vector<candle*> * candles, int offset);
	void buy(std::string ticker, float stock_price, float loss_limit_price);
	void sell(position * p);
};

#endif

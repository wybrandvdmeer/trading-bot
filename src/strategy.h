#include <vector>

#include "logger.h"
#include "candle.h"
#include "db_api.h"
#include "indicators.h"
#include "alpaca_api.h"

#define MAX_LAG_TIME 120
#define SMA_50_200_POSITIVE_TREND_LENGTH 3

#ifndef STRATEGY_H
#define STRATEGY_H

class strategy {
public:
	virtual bool trade(std::string ticker, 
		std::vector<candle*> * candles, 
		candle * candle,
		float max_delta_close_sma_200,
		bool back_testing)=0;
	bool disable_alpaca=false, finished_for_the_day=false;
protected:
	logger log;
	db_api * db;
	indicators * ind;
	alpaca_api alpaca;
	bool candle_in_nse_closing_window(candle * c);
	bool in_openings_window(long current_time);
	void sell(position *p);
	void buy(std::string ticker, float stock_price, long buy_time);
	int find_position_of_last_day(std::vector<candle*> *candles);
};

#endif

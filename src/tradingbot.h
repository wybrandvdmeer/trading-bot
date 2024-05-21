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
	bool force, debug, disable_alpaca;
private:
	float prv_macd_signal_diff;
	float sma_200;
	top_gainers tg;
	yahoo_api yahoo;
	db_api db;
	indicators ind;
	logger log;
	alpaca_api alpaca;
	bool trade(std::vector<candle*> * candles);
	bool nse_is_open();
	bool candle_in_nse_closing_window(candle * c);
	void buy(std::string ticker, float stock_price, long buy_time);
	void sell(position * p);
	bool get_quality_candles(std::vector<candle*> *candles);
	bool candle_in_openings_pause(candle *c);
	void finish(std::string ticker, std::vector<candle*> *candles, float sma_200);
	std::string date_to_string(long ts);
	void ema_test();
};

#endif

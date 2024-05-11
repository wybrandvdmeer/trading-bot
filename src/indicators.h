#include <vector>

#include "logger.h"
#include "macd.h"

#ifndef INDICATORS_H
#define INDICATORS_H

class indicators {
public:
	macd * calculate_macd(std::vector<candle*> * candles);
	float calculate_ema(int noOfDays, std::vector<candle*> * candles, int offset);
	float calculate_sma(int noOfDays, std::vector<candle*> * candles, int offset);
private:
	std::vector<float> calculate_ema(int noOfDays, std::vector<candle*> * candles);
	logger log;
};

#endif

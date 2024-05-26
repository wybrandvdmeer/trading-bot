#include <vector>

#include "logger.h"
#include "macd.h"

#ifndef INDICATORS_H
#define INDICATORS_H

class indicators {
public:
	float * custom_ind[3];
	float sma_200;
	macd m;
	void calculate_macd(std::vector<float> prices);
	float calculate_ema(int no_of_days, std::vector<float> prices);
	float calculate_sma(int no_of_days, std::vector<float> prices);
private:
	void calculate_ema(int no_of_days, std::vector<float> prices, std::vector<float> * emas);
	logger log;
};

#endif

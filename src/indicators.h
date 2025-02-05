#include <vector>
#include <string>

#include "logger.h"
#include "macd.h"

#ifndef INDICATORS_H
#define INDICATORS_H

class indicators {
public:
	float * custom_ind[3];
	std::vector<float> sma_200;
	std::vector<float> sma_50;
	macd m;
	void calculate_macd(std::vector<float> prices);
	float calculate_ema(int no_of_days, std::vector<float> prices);
	float calculate_sma(int no_of_days, std::vector<float> prices);
	void calculate_sma_200(std::vector<float> prices);
	void calculate_sma_50(std::vector<float> prices);
	float get_sma_200(int offset);
	float get_sma_50(int offset);
	float get_sma_diff_50_200(int offset);
	void reset(std::string ticker);
	bool is_sma_50_200_diff_trending(int length, bool positive_trend);
private:
	std::string ticker;
	void calculate_ema(int no_of_days, std::vector<float> prices, std::vector<float> * emas);
	void calculate_sma(int no_of_days, std::vector<float> prices, std::vector<float> * smas);
	logger log;
};

#endif

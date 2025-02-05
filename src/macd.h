#include <vector>

#include "logger.h"

#ifndef MACD_H
#define MACD_H

class macd {
public:
	float get_ema_12(int offset);
	float get_ema_26(int offset);
	float get_signal(int offset);
	float get_macd(int offset);
	float get_histogram(int offset);
	bool is_histogram_trending(int length, bool positive_trend);
	bool is_histogram_trending(int length, int offset, bool positive_trend);
	std::vector<float> ema_12;
	std::vector<float> ema_26;
	std::vector<float> macd;
	std::vector<float> signal;
private:
	logger log;
};

#endif

#include <vector>

#ifndef MACD_H
#define MACD_H

class macd {
public:
	float get_ema_12(int offset);
	float get_ema_26(int offset);
	float get_signal(int offset);
	float get_macd(int offset);
	float get_histogram(int offset);
	bool is_histogram_trend_positive(int length);
	std::vector<float> ema_12;
	std::vector<float> ema_26;
	std::vector<float> macd;
	std::vector<float> signal;
};

#endif

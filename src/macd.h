#include <vector>

#ifndef MACD_H
#define MACD_H

class macd {
public:
	float get_ema_12(int offset);
	float get_ema_26(int offset);
	float get_signal(int offset);
	float get_macd(int offset);
	std::vector<float> ema_12;
	std::vector<float> ema_26;
	std::vector<float> macd;
	std::vector<float> signal;
};

#endif

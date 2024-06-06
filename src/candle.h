#include <string>

#ifndef CANDLE_H
#define CANDLE_H

class candle {
public:
	int id;
	long time, volume;
	float open, close, low, high, adjustedClose;
	candle(long time=0, float open=0.0, float close=0.0, float low=0.0, float high=0.0, float adjustedClose=0.0, long volume=0);
	bool is_valid();
};

#endif

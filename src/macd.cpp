#include <vector>

#include "macd.h"

using namespace std;

float macd::get_ema_12(int offset) {
	return ema_12.at(ema_12.size() - offset - 1);
}

float macd::get_ema_26(int offset) {
	return ema_26.at(ema_26.size() - offset - 1);
}

float macd::get_signal(int offset) {
	return signal.at(signal.size() - offset - 1);
}

float macd::get_macd(int offset) {
	return macd.at(macd.size() - offset - 1);
}

float macd::get_histogram(int offset) {
	return get_macd(offset) - get_signal(offset);
}

bool macd::is_histogram_trend_negative(int offset, int length) {
	bool up=false;
	float x=get_histogram(offset);
	for(int idx=1; idx < length; idx++) {
		up = x > get_histogram(offset + idx);
		x = get_histogram(offset + idx);
	}

	return !up;
}


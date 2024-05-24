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

bool macd::is_histogram_trend_positive(int length) {
	bool up=true;
	for(int idx=0; up && idx < length && idx < macd.size() - 1; idx++) {
		up = get_histogram(idx) > get_histogram(idx + 1);
	}

	return up;
}


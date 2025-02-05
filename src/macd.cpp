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

bool macd::is_histogram_trending(int length, bool positive_trend) {
	return is_histogram_trending(length, 0, positive_trend);
}

bool macd::is_histogram_trending(int length, int offset, bool positive_trend) {
	for(int idx=0; idx < length && idx < macd.size() - offset; idx++) {
		if(positive_trend && get_histogram(idx + offset) <= get_histogram(idx + offset + 1)) {
			return false;
		}

		if(!positive_trend && get_histogram(idx + offset) >= get_histogram(idx + offset + 1)) {
			return false;
		} 
	}

	return true;
}


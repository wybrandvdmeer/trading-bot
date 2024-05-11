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


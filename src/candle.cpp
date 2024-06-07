#include "candle.h"

using namespace std;

candle::candle(
	long time, 
	float open, 	
	float close, 
	float low, 	
	float high, 
	long volume,
	float adjustedClose) {
	candle::time = time;
	candle::open = open; 	
	candle::close = close; 
	candle::low = low; 	
	candle::high = high;
	candle::volume = volume;
	candle::adjustedClose = adjustedClose;
}

bool candle::is_green() {
	return close > open;
}

bool candle::is_red() {
	return close <= open;
}

bool candle::equals(candle c) {
	return time == c.time &&
		open == c.open &&
		close == c.close &&
		low == c.low &&
		high == c.high &&
		volume == c.volume;
}

bool candle::is_valid() {
	// The last yahoo candle has always the same values.
	return open > 0 && close > 0 && !(open == close && close == low && low == high);
}

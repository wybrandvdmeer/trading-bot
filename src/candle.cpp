#include <ctime>

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

bool candle::is_green(bool inclusive) {
	return inclusive ? close >= open : close > open;
}

bool candle::is_red() {
	return close < open;
}

bool candle::is_red(bool inclusive) {
	return inclusive ? close <= open : close < open;
}

bool candle::no_price_change() {
	return open == close;
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

std::string candle::get_time_string() {
    time_t ts = time - 4 * 3600;
    struct tm *t = gmtime(&ts);
    char buf[100];
    sprintf(buf, "%02d:%02d:%02d",
        t->tm_hour,
        t->tm_min,
        t->tm_sec);
    return std::string(buf);
}


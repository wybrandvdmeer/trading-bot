#include "candle.h"

using namespace std;

candle::candle(
	long time, 
	float open, 	
	float close, 
	float low, 	
	float high, 
	float adjustedClose, 
	long volume) {
	candle::time =  time;
	candle::open = open; 	
	candle::close = close; 
	candle::low = low; 	
	candle::high = high;
	candle::adjustedClose = adjustedClose;
	candle::volume = volume;
}

bool candle::is_valid() {
	return open > 0 && close > 0;
}

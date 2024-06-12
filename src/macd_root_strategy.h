#include <vector>

#include "strategy.h"

#ifndef MACD_ROOT_STRATEGY
#define MACD_ROOT_STRATEGY

class macd_root_strategy : public strategy {
public:
	macd_root_strategy(db_api * db, indicators *ind);
	bool trade(std::string ticker, 
		std::vector<candle*> * candles, 
		candle * candle,
		float max_delta_close_sma_200,
		bool back_testing);
private:
	bool low_detected(std::vector<candle*> *candles);
	bool wait_for_macd_crossing=false;
};

#endif

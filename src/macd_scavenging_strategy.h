#include <vector>

#include "strategy.h"

#ifndef MACD_SCAVENGING_STRATEGY
#define MACD_SCAVENGING_STRATEGY

class macd_scavenging_strategy : public strategy {
public:
	macd_scavenging_strategy(db_api * db, indicators *ind);
	bool trade(std::string ticker, 
		std::vector<candle*> * candles, 
		candle * candle,
		float max_delta_close_sma_200,
		bool back_testing);
private:
	float get_macd_set_point(macd m, std::vector<candle*> *candles);
};

#endif

#include <vector>

#include "strategy.h"

#ifndef DIP_AND_RIP_STRATEGY
#define DIP_AND_RIP_STRATEGY

class dip_and_rip_strategy : public strategy {
public:
	dip_and_rip_strategy(db_api * db, indicators *ind);
	bool trade(std::string ticker, 
		std::vector<candle*> * candles, 
		candle * candle,
		float max_delta_close_sma_200,
		bool back_testing);
private:
};

#endif

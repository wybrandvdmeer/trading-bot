#include <vector>

#include "strategy.h"

#ifndef SMA_CROSSOVER_STRAT
#define SMA_CROSSOVER_STRAT

class sma_crossover_strategy : public strategy {
public:
	sma_crossover_strategy(db_api * db, indicators *ind);
	bool trade(std::string ticker, 
		std::vector<candle*> * candles, 
		candle * candle,
		float max_delta_close_sma_200,
		bool back_testing);
private:
	bool in_second_positive_sma_period(std::vector<candle*> * candles);
};

#endif

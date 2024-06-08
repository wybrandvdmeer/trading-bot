#include "strategy.h"
#include "dip_and_rip_strategy.h"
#include "position.h"

#define SELL_NEGATIVE_TREND_LENGTH 5
#define BUY_POSITIVE_TREND_LENGTH 2
#define RELATIVE_HIST 0.3 // Results in getting only the first trade.

using namespace std;

dip_and_rip_strategy::dip_and_rip_strategy(db_api * db, indicators *ind) {
	strategy::db = db;
	strategy::ind = ind;
}

bool dip_and_rip_strategy::trade(std::string ticker, 
	std::vector<candle*> *candles, 
	candle *candle,
	float max_delta_close_sma_200,
	bool back_testing) {
	float open_0 = candle->open;
	float close_0 = candle->close;

	if(finished_for_the_day) {
		return true;
	}

	finished_for_the_day = candle_in_nse_closing_window(candle);
	
	position * p = db->get_open_position(ticker);
	if(p != NULL) {
		bool bSell = false;
		if(finished_for_the_day) {
			log.log("sell: current candle is in closing window. Trading day is finished.");
			bSell = true;
		} else
		if(close_0 >= p->sell_off_price) {
			p->stop_loss_activated = 0;
			log.log("sell: current price (%f) is above selling price (%f)." ,
				close_0 , p->sell_off_price);
			bSell = true;
		} else
		if(close_0 <= p->loss_limit_price) {
			p->stop_loss_activated = 1;
			log.log("sell: current price (%f) is below stop-loss price (%f)." ,
				close_0 , p->loss_limit_price);
			bSell = true;
		}

		if(bSell) {
			p->sell = candle->time;
			p->sell_price = close_0;
			sell(p);
		}

		return finished_for_the_day;
	}
	
	/* Buy logic. 
	*/
	if(in_openings_window(candle->time)) { // for back-testing.
 		log.log("no trade: Candle is still in openings window."); 
	} else
	if(candle->is_red()) {
 		log.log("no trade: candle is red."); 
	} else
	if(!back_testing && time(0) - candle->time >= MAX_LAG_TIME) {
		log.log("no trade: difference (%ld) candle time (%ld) vs current-time (%ld) is too great.",
			time(0) - candle->time,
			candle->time,
			time(0));
	} else {
		buy(ticker, close_0, candle->time);
	}

	return false;
}

#include "strategy.h"
#include "dip_and_rip_strategy.h"
#include "position.h"

#define SEARCH_HISTORY 4

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

	if(wait_for_macd_crossing && ind->m.get_histogram(0) > 0) {
		log.log("Signal of candle (%s) crossed the macd line.", candle->get_time_string());
		wait_for_macd_crossing = false;
	}

	finished_for_the_day = candle_in_nse_closing_window(candle);
	
	position * p = db->get_open_position(ticker);
	if(p != NULL) {
		bool bSell = false;
		if(wait_for_macd_crossing && candle->is_red()) {
			log.log("sell: red candle while waiting for macd zero crossing.");
			bSell = true;
		} else
		if(!wait_for_macd_crossing && strategy::ind->m.get_macd(0) <= strategy::ind->m.get_signal(0)) {
            log.log("sell: macd (%f) is below signal (%f)." ,
                strategy::ind->m.get_macd(0) , strategy::ind->m.get_signal(0));
            bSell = true;
		} else
		if(finished_for_the_day) {
			log.log("sell: current candle is in closing window. Trading day is finished.");
			bSell = true;
		} else
		if(close_0 >= p->sell_off_price) {
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
			wait_for_macd_crossing = false;
		}

		return finished_for_the_day;
	}
	
	/* Buy logic. 
	*/
	if(in_openings_window(candle->time)) { // for back-testing.
 		log.log("no trade: Candle is still in openings window."); 
	} else
	if(!ind->m.is_histogram_trending(SEARCH_HISTORY, 1, false)) {
 		log.log("no trade: histogram is not negatively trending."); 
	} else 
	if(!low_detected(candles)) {
 		log.log("no trade: no low detected."); 
	} else
	if(!back_testing && time(0) - candle->time >= MAX_LAG_TIME) {
		log.log("no trade: difference (%ld) candle time (%ld) vs current-time (%ld) is too great.",
			time(0) - candle->time,
			candle->time,
			time(0));
	} else {
		buy(ticker, close_0, candle->time);
		wait_for_macd_crossing = true;
	}

	return false;
}

bool dip_and_rip_strategy::low_detected(vector<candle *> *candles) {
	if(candles->at(candles->size() - 1)->is_red(true)) {
		return false;
	}

	for(int idx=0; idx < SEARCH_HISTORY; idx++) {
		if(candles->at(candles->size() - 2 - idx)->is_green(true)) {
			return false;
		}
	}

	return true;
}





#include "strategy.h"
#include "macd_root_strategy.h"
#include "position.h"

#define SEARCH_HISTORY 4

/* strategie is erop gericht om in te haken op de low's, anticiperen dat
de macd positief wordt en dan te traden totdat de macd weer negatief word.
close moet boven sma-200 zijn, om false low's zoveel mogelijk te vermijden.
*/

using namespace std;

macd_root_strategy::macd_root_strategy(db_api * db, indicators *ind) {
	strategy::db = db;
	strategy::ind = ind;
}

bool macd_root_strategy::trade(std::string ticker, 
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
		if(ind->m.get_histogram(0) <= 0 && red_top(candles)) {
			log.log("sell: red candles while waiting for macd zero crossing.");
			bSell = true;
		} else
		if(ind->m.get_histogram(1) > 0 && ind->m.is_histogram_trending(2, 0, false)) {
            log.log("sell: histogram is trending negatively." ,
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
		}

		return finished_for_the_day;
	}

	/* Buy logic. 
	*/
	if(close_0 <= ind->get_sma_200(0)) {
 		log.log("no trade: price <= sma_200."); 
	} else
	if(ind->m.get_histogram(0) >= 0) {
 		log.log("no trade: macd above signal."); 
	} else
	if(!low_detected(candles)) {
 		log.log("no trade: no low detected."); 
	} else
	if(in_openings_window(candle->time)) { // for back-testing.
 		log.log("no trade: Candle is still in openings window."); 
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

bool macd_root_strategy::red_top(std::vector<candle*> *candles) {
	return 
		candles->size() >= 2 && 
		candles->at(candles->size() - 1)->is_red() && 
		candles->at(candles->size() - 2)->is_red();
}

bool macd_root_strategy::low_detected(vector<candle *> *candles) {
	if(!candles->at(candles->size() - 1)->is_green()) {
		return false;
	}

	for(int idx=0; idx < SEARCH_HISTORY; idx++) {
		if(!candles->at(candles->size() - 2 - idx)->is_red()) {
			return false;
		}
	}

	return true;
}





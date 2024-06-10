#include "strategy.h"
#include "macd_scavenging_strategy.h"
#include "position.h"

#define SELL_NEGATIVE_TREND_LENGTH 5
#define BUY_POSITIVE_TREND_LENGTH 2
#define RELATIVE_HIST 0.3 // Results in getting only the first trade.

using namespace std;

macd_scavenging_strategy::macd_scavenging_strategy(db_api * db, indicators *ind) {
	strategy::db = db;
	strategy::ind = ind;
}

bool macd_scavenging_strategy::trade(std::string ticker, 
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
		if(strategy::ind->m.get_macd(0) <= strategy::ind->m.get_signal(0)) {
			p->stop_loss_activated = 0;
			log.log("sell: macd (%f) is below signal (%f)." ,
				strategy::ind->m.get_macd(0) , strategy::ind->m.get_signal(0));
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
	
	float macd_set_point = get_macd_set_point(strategy::ind->m, candles);

	/* Buy logic. 
	*/
	if(in_openings_window(candle->time)) { // for back-testing.
 		log.log("no trade: Candle is still in openings window."); 
	} else
	if(candle->is_red()) {
 		log.log("no trade: candle is red."); 
	} else
	if(strategy::ind->m.get_macd(0) <= strategy::ind->m.get_signal(0) + macd_set_point) {
		log.log("no trade: macd(%f) is smaller then signal (%f).",
		strategy::ind->m.get_macd(0), 
		strategy::ind->m.get_signal(0));
	} else
	if(!strategy::ind->m.is_histogram_trending(BUY_POSITIVE_TREND_LENGTH, true)) {
		log.log("no trade: not a positive trend on the macd histogram.");
	} else 
	if(close_0 < strategy::ind->get_sma_200(0) + max_delta_close_sma_200) {
		log.log("no trade: price (%f) is below sma200 (%f + %f).", 
			close_0, 
			strategy::ind->get_sma_200(0), 
			max_delta_close_sma_200);
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

float macd_scavenging_strategy::get_macd_set_point(macd m, std::vector<candle*> *candles) {
	int start = find_position_of_last_day(candles);
	if(start == -1) {
		log.log("Cannot find position of last day.");
		return 0;
	}

	float max_hist=0;
	for(int idx=start; idx < candles->size(); idx++) {
		int day = candles->size() - idx;
		if(max_hist < m.get_histogram(day)) {
			max_hist = m.get_histogram(day);
		}
	}

	log.log("Max histogram: %f", candles->size(), max_hist);

	return RELATIVE_HIST * max_hist;
}

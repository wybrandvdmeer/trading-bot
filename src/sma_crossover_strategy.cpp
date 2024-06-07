#include "strategy.h"
#include "sma_crossover_strategy.h"
#include "position.h"

using namespace std;

sma_crossover_strategy::sma_crossover_strategy(db_api * db, indicators *ind) {
	strategy::db = db;
	strategy::ind = ind;
}

bool sma_crossover_strategy::trade(std::string ticker, 
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
		if(strategy::ind->get_sma_50(0) - strategy::ind->get_sma_200(0) <= 0) { 
			log.log("sell: sma_50 is below sma_200.");
			bSell = true;
		} else
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
			p->sell_price = close_0;

			// In case of back-testing, the candle time is leading. 
			if(back_testing) {
				p->sell = time(0);
			} else {
				p->sell = candle->time;
			}
			sell(p);

			// One shot per day. 
			finished_for_the_day = true;
		}

		return finished_for_the_day;
	}

	// Buy logic. 
	if(in_openings_window(candle->time)) { // for back-testing.
 		log.log("no trade: Candle is still in openings window."); 
	} else
	if(candle->is_red()) {
		log.log("no trade: red candle.");
	} else
	if(!strategy::ind->is_sma_50_200_diff_trending(SMA_50_200_POSITIVE_TREND_LENGTH, true)) {
		log.log("no trade: not a positive trend on the sma-50/200 indicators.");
	} else 
	if(close_0 < strategy::ind->get_sma_200(0) + max_delta_close_sma_200) {
		log.log("no trade: price (%f) is below sma200 (%f + %f).", 
			close_0, strategy::ind->get_sma_200(0), max_delta_close_sma_200);
	} else
	if(strategy::ind->get_sma_200(0) >= strategy::ind->get_sma_50(0)) {
		log.log("no trade: sma_200(%f) is greater then sma_50(%f).",
			strategy::ind->get_sma_200(0), 
			strategy::ind->get_sma_50(0));
	} else
	if(!in_second_positive_sma_period(candles)) {
		log.log("no trade: not in second positive sma period.");
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

bool sma_crossover_strategy::in_second_positive_sma_period(std::vector<candle*> * candles) {
	int start = find_position_of_last_day(candles);
	if(start == -1) {
		log.log("Cannot find position of last day.");
		return false;
	}
	int period=0;

	bool sma_positive = false;
	for(int idx=start; idx < strategy::ind->sma_50.size(); idx++) {
		if(strategy::ind->sma_50.at(idx) > strategy::ind->sma_200.at(idx)) {
			if(!sma_positive) {
				sma_positive = true;
				period++;
			}
		} else {
			sma_positive = false;
		}
	}

	return period == 2 && sma_positive;
}

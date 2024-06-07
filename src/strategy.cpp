#include <ctime>

#include "candle.h"
#include "strategy.h"

#define OPENINGS_WINDOW_IN_MIN 10

bool strategy::candle_in_nse_closing_window(candle * c) {
    time_t now = time(NULL);
    struct tm *tm_struct = gmtime(&now);

    tm_struct->tm_hour = 19;
    tm_struct->tm_min = 45;
    tm_struct->tm_sec = 0;

    long ts = timegm(tm_struct)%(24 * 3600);
    long ts_candle = (c->time)%(24 * 3600);

    return ts <= ts_candle && ts_candle < ts + 15 * 60;
}

bool strategy::in_openings_window(long current_time) {
	time_t now = time(NULL);
	struct tm *tm_struct = gmtime(&now);

	tm_struct->tm_hour = 13;
	tm_struct->tm_min = 30;
	tm_struct->tm_sec = 0;

	long ts = timegm(tm_struct)%(24 * 3600);
	current_time = current_time%(24 * 3600);

	return ts <= current_time && current_time < ts + OPENINGS_WINDOW_IN_MIN * 60;
}

int strategy::find_position_of_last_day(std::vector<candle*> *candles) {
	int idx=0, day_break_position=-1, first_day, prv_day=-1;
    for(auto c : *candles) {
    	first_day = gmtime(&(c->time))->tm_wday;
        if(prv_day == -1) {
        	prv_day = first_day;
        } else
       	if(prv_day != first_day) {
        	day_break_position = idx;
            prv_day = first_day;
		}
        idx++;
	}
	return day_break_position;
}

void strategy::sell(position *p) {
	log.log("Sell (%s), price: %f, number: %d, sell_price: %f, loss-limit: %f, stop_loss_activated: %d", 
		p->ticker.c_str(), 
		p->stock_price,
		p->no_of_stocks,
		p->sell_price,
		p->loss_limit_price,
		p->stop_loss_activated);

	if(disable_alpaca || alpaca.close_position(*p)) {
		db->close_position(*p);
	}
}

void strategy::buy(std::string ticker, float stock_price, long buy_time) {
	position p;
	p.ticker = ticker;
	p.no_of_stocks = (int)(200.0/stock_price);
	p.stock_price = stock_price;
	p.buy = buy_time;

	if(p.no_of_stocks <= 0) {
		log.log("No-of-stocks calculated is 0.");
		return;
	}

	p.loss_limit_price = stock_price - (stock_price/100) * 2;	

	// 3 percent gain is reasonable.
	p.sell_off_price = stock_price + (stock_price/100) * 3;	

	log.log("Buy (%s), price: %f, number: %d, sell_off_price: %f, loss-limit: %f", 
		ticker.c_str(), 
		p.stock_price,
		p.no_of_stocks,
		p.sell_off_price,
		p.loss_limit_price);

	if(disable_alpaca || alpaca.open_position(p)) {
		db->open_position(p);
	}
}

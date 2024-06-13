#include <string>

#ifndef POSITION_H
#define POSITION_H

class position {
public:
	std::string ticker;
	std::string alpaca_order_id;
	std::string alpaca_order_status;
	long buy, sell;
	int id, no_of_stocks;
	float stock_price, sell_price, sell_off_price, loss_limit_price;
	bool stop_loss_activated=false;
};

#endif

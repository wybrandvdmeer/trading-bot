#include <string>

#ifndef POSITION_H
#define POSITION_H

class position {
public:
	std::string ticker;
	long buy, sell;
	int no_of_stocks;
	float stock_price, sell_off_price, loss_limit_price;
	bool stop_loss_activated;
};

#endif

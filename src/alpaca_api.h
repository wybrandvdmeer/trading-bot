#ifndef ALPACA_API_H
#define ALPACA_API_H

class alpaca_api {
public:
	bool open_position(position &p);
	bool close_position(position &p);
private:
	logger log;
};

#endif

#include <string>
#include <vector>

#include "logger.h"
#include "download.h"

#ifndef TOP_GAINERS_H
#define TOP_GAINERS_H

class top_gainers {
public:
	top_gainers();
	std::vector<std::string> * get(std::vector<std::string> black_listed_tickers);
	bool slave;
private:
	logger log;
	download dl;
	std::vector<std::string> split(const std::string &s);
	std::vector<std::string> * yget(std::vector<std::string> black_listed_tickers);
	std::string get_top_gainers_list_name();
};

#endif

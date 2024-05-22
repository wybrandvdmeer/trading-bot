#include <string>
#include <vector>

#include "logger.h"
#include "download.h"

#ifndef TOP_GAINERS_H
#define TOP_GAINERS_H

class top_gainers {
public:
	std::vector<std::string> * get(std::vector<std::string> black_listed_tickers);
private:
	logger log;
	download dl;
	std::vector<std::string> split(const std::string &s);
};

#endif

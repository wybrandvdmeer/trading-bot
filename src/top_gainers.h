#include <string>
#include <vector>

#include "logger.h"
#include "download.h"

#ifndef TOP_GAINERS_H
#define TOP_GAINERS_H

class top_gainers {
public:
	top_gainers();
	std::vector<std::string> * get();
private:
	logger log;
	download dl;
	std::vector<std::string> split(const std::string &s);
	std::vector<std::string> * yget();
};

#endif

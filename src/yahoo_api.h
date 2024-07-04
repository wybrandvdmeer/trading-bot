#include <vector>
#include <string>

#include "stock.h"
#include "candle.h"
#include "download.h"
#include "logger.h"

#ifndef YAHOO_API_H
#define YAHOO_API_H

class yahoo_api {
public:
	yahoo_api();
	download dl;
	stock * info(std::string ticker);
	std::vector<candle*> * stockPrices(std::string ticker, std::string interval, std::string range);
	bool debug;
private:
	std::string getModuleString();
	logger log;
};

#endif

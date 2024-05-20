#include <string>
#include <iostream>
#include <set>
#include <vector>

#include <fstream>
#include <cstdlib>

#include "nlohmann/json.hpp"

#include "candle.h"
#include "yahoo_api.h"

using namespace std;

yahoo_api::yahoo_api() {
	yahoo_api::debug = false;
}

std::string yahoo_api::getModuleString() {
	set<std::string> modules;
	// modules.insert("financialData");
	// modules.insert("assetProfile");   
	// modules.insert("summaryDetail");
    modules.insert("quoteType");
	modules.insert("defaultKeyStatistics");

	std::string moduleString="modules=";
	std::set<std::string>::iterator it;
	for(it = modules.begin(); it != modules.end(); ++it) {
		moduleString.append(*it);
		if(it != modules.end()) {
			moduleString.append("%2C");
		}
	}
	return moduleString;
}	

stock * yahoo_api::info(std::string ticker) {
	std::string url = "https://query2.finance.yahoo.com/v10/finance/quoteSummary/" + ticker +
		"?" + getModuleString() + "&corsDomain=finance.yahoo.com&formatted=false&symbol=" + ticker;
	
	std::string response = dl.request(url);

    auto json = nlohmann::json::parse(response);

	long outstandingShares;
	std::string exchange;
	for (auto r : json["quoteSummary"]["result"]) {
		outstandingShares = r["defaultKeyStatistics"]["sharesOutstanding"];
		exchange = r["quoteType"]["exchange"];
	}

	stock * s = new stock();
	s->outstandingShares = outstandingShares;
	s->exchange = exchange;

	return s;
}

std::vector<candle*>* yahoo_api::stockPrices(
	std::string ticker, 
	std::string interval, 
	std::string range) {
	std::string url = "https://query2.finance.yahoo.com/v8/finance/chart/" + ticker + "?range=" + range 
		+ "&interval=" + interval + "&includePrePost=True&events=div%2Csplits%2CcapitalGains";

	std::string response = dl.request(url);
	if(yahoo_api::debug) {
		cout << response.c_str();
	}

	if(response.length() == 0) {
		return NULL;
	}

	auto json = nlohmann::json::parse(response);

	std::vector<candle*> * candles = new std::vector<candle*>();
	long ts;

	for (auto r : json["chart"]["result"]) {
		for(auto rr : r["timestamp"]) {
			candle * c = new candle(rr); 
			candles->push_back(c);
		}

		// If there is a lot of data, then its possible there is no adjustedClose.
		for(auto rr: r["indicators"]["adjclose"]) {
			int idx=0;
			for(auto rrr : rr["adjclose"]) {
				if(!rrr.is_null()) {
					candles->at(idx)->adjustedClose = rrr;
				}
				idx++;
			}
		}

		for(auto rrr: r["indicators"]["quote"]) {
			int idx=0;
			for(auto rrrr : rrr["open"]) {
				if(!rrrr.is_null()) {
					candles->at(idx)->open = rrrr;
				}
				idx++;
			}
			idx=0;
			for(auto rrrr : rrr["close"]) {
				if(!rrrr.is_null()) {
					candles->at(idx)->close = rrrr;
				}
				idx++;
			}
			idx=0;
			for(auto rrrr : rrr["low"]) {
				if(!rrrr.is_null()) {
					candles->at(idx)->low = rrrr;
				}
				idx++;
			}
			idx=0;
			for(auto rrrr : rrr["high"]) {
				if(!rrrr.is_null()) {
					candles->at(idx)->high = rrrr;
				}
				idx++;
			}
			idx=0;
			for(auto rrrr : rrr["volume"]) {
				if(!rrrr.is_null()) {
					candles->at(idx)->volume = rrrr;
				}
				idx++;
			}
		}
	}

	return candles;
}










#include <sstream>
#include <vector>
#include <string>
#include <algorithm>
#include <map>

#include "top_gainers.h"
#include "logger.h"


using namespace std;

top_gainers::top_gainers() {
}

std::vector<std::string> * top_gainers::get() {
	return yget();
}

std::vector<std::string> * top_gainers::yget() {
	std::map<std::string, float> ticker_prices;

	vector<std::string> * top_gainers = new vector<std::string>();

	std::string response = dl.request_bin_data("https://finance.yahoo.com/gainers?count=100&offset=0");
	if(response.length() == 0) {
		log.log("No data received");
		return top_gainers;
	}

	vector<std::string> words = split(response);

	std::string ticker;
	float price;
	for(std::vector<std::string>::iterator it = words.begin(); it != words.end(); it++) {
		price = -1;
		if ((*it).rfind("href=\"/quote/", 0) == 0 && (*it).find('%') == std::string::npos) {
			ticker = (*it).substr(13, (*it).length() - 13 - 1);
		}

		if (
			(*it).rfind("active=\"\">", 0) == 0 &&
			(*it).rfind("M<") == std::string::npos &&
			(*it).rfind("B<") == std::string::npos &&
			(*it).rfind("active=\"\"><span") == std::string::npos) {
			std::string price_string = (*it).substr(10);
		
			price_string = price_string.substr(0, price_string.rfind("</fin-streamer"));
			price_string.erase(
				std::remove(price_string.begin(), price_string.end(), ','), 
				price_string.end()); // Remove ','

			price = std::stof(price_string, NULL);
		}
		
		if(price != -1 && !ticker.empty()) {	
			if(ticker_prices.count(ticker) == 0) {
				ticker_prices[ticker] =  price;
			}
			price = -1;
		}
	}

	for(std::map<std::string,float>::iterator it = ticker_prices.begin(); 
		it != ticker_prices.end(); it++) {
		if(it->second <= 20) {
			log.log("%s -> %f", it->first.c_str(), it->second);	
  			top_gainers->push_back(it->first);
		}
	}

	return top_gainers->size() > 0 ? top_gainers : NULL;
}

std::vector<std::string> top_gainers::split(const std::string &s) {
    std::stringstream ss(s);
    std::string item;
    std::vector<std::string> elems;
    while (std::getline(ss, item, ' ')) {
        elems.push_back(item);
    }
    return elems;
}

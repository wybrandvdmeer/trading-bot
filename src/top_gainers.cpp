#include <iostream>
#include <sstream>
#include <vector>
#include <string>

#include "top_gainers.h"
#include "logger.h"

using namespace std;

std::vector<std::string> * top_gainers::get() {
	std::string response = dl.request_bin_data("https://finance.yahoo.com/gainers?count=100&offset=0");
	vector<std::string> words = split(response);

	vector<std::string> * top_gainers = new vector<std::string>();

	for(std::vector<std::string>::iterator it = words.begin(); it != words.end(); it++) {
		if ((*it).rfind("href=\"/quote/", 0) == 0 && (*it).find('%') == std::string::npos) {
			top_gainers->push_back((*it).substr(13, (*it).length() - 13 - 1));
		}
	}

	return top_gainers;
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

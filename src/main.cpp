#include <iostream>
#include <algorithm>
#include <string>
#include <thread>
#include <chrono>
#include <ctime>

#include "logger.h"
#include "top_gainers.h"
#include "tradingbot.h"

using namespace std;

char const *USAGE="td\n";

char* getOption(char ** begin, char ** end, const std::string & option) {
    char ** itr = std::find(begin, end, option);
    if (itr != end) {
        return *itr;
    }
    return NULL;
}

char* getOptionValue(char ** begin, char ** end, const std::string & option) {
    char ** itr = std::find(begin, end, option);
    if (itr != end && ++itr != end) {
        return *itr;
    }
    return NULL;
}

int main(int argc, char ** argv) {
	logger log;
	int top_gainer_position = 0;
	tradingbot t;
			
	log.log("Starting bot.");
	
	char *ticker = getOptionValue(argv, argv + argc, "--ticker");
	if(ticker != NULL) {
		t.ticker = std::string(ticker);
	}

	char *tgp = getOptionValue(argv, argv + argc, "--top-gainer-position");
	if(tgp != NULL) {
		top_gainer_position = atoi(tgp);
	}

	char *forceOption = getOption(argv, argv + argc, "--force");
	if(forceOption != NULL) {
		t.force = true;
	}
	
	char *debug_option = getOption(argv, argv + argc, "--debug");
	if(debug_option != NULL) {
		t.debug = true;
	}
	
	t.trade(0, top_gainer_position);
}

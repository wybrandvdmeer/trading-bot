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
	bool force=false;
	int top_gainer_position = 0;
	tradingbot t;
			
	log.log("Starting bot.");

	char *tgp = getOptionValue(argv, argv + argc, "--top-gainer-position");
	if(tgp != NULL) {
		top_gainer_position = atoi(tgp);
	}

	char *forceOption = getOption(argv, argv + argc, "--force");
	if(forceOption != NULL) {
		force = true;
	}
	
	t.trade(0, force, top_gainer_position);
}

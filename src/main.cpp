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

bool nse_is_open() {
	time_t now = time(NULL);
	struct tm *tm_struct = localtime(&now);

	int hour = tm_struct->tm_hour;
	int minutes = tm_struct->tm_min;

	if(hour >= 15 && hour < 22) {
		if(hour == 15 && minutes <= 30) {
			return false;
		}
		return true;
	}
	return false;
}

int main(int argc, char ** argv) {

	top_gainers tg;
	vector<std::string> * top_gainers = NULL;
	tradingbot *t;
	logger log;
	bool force=false;
	int top_gainer_position = 0;
			
	log.log("Starting bot.");

	char *tgp = getOptionValue(argv, argv + argc, "--top-gainer-position");
	if(tgp != NULL) {
		top_gainer_position = atoi(tgp);
	}

	char *forceOption = getOption(argv, argv + argc, "--force");
	if(forceOption != NULL) {
		force = true;
	}

	while(true) {
		if(!force && !nse_is_open()) {
			std::this_thread::sleep_for(std::chrono::milliseconds(5 * 1000));
			continue;
		}

		if(top_gainers == NULL) {
			top_gainers = tg.get();
		
			if(top_gainers->size() == 0) {
				cout << "No top gainers.";
				exit(1);
			}

			tradingbot tb(top_gainers->at(top_gainer_position));
			tb.configure();
			t = &tb;
		}

		t->trade(0);

		std::this_thread::sleep_for(std::chrono::milliseconds(60 * 1000));
	}

	exit(0);
}

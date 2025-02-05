#include <iostream>
#include <algorithm>
#include <string>
#include <thread>
#include <chrono>
#include <ctime>

#include "glog/logging.h"

#include "logger.h"
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
	tradingbot t;

	google::InitGoogleLogging("tradebot");

	char *ticker = getOptionValue(argv, argv + argc, "--ticker");
	if(ticker != NULL) {
		t.ticker = std::string(ticker);
	}

	char *id = getOptionValue(argv, argv + argc, "--id");
	if(id != NULL) {
		t.id = std::stoi(id);
	}

	char *strategy = getOptionValue(argv, argv + argc, "--strategy");
	if(strategy != NULL) {
		std:string s = std::string(strategy);
		if(s != "macd" && s != "sma" && s != "rmcd") {
			log.log("Strategy should be macd, rmcd, or sma.");
			exit(1);
		}
		t.strategy = std::string(strategy);
	}

	char *db_file = getOptionValue(argv, argv + argc, "--db-file");
	if(db_file != NULL) {
		t.db_file = std::string(db_file);
		logger::enable_date = false;
	}

	char *forceOption = getOption(argv, argv + argc, "--force");
	if(forceOption != NULL) {
		t.force = true;
	}
	
	char *debug_option = getOption(argv, argv + argc, "--debug");
	if(debug_option != NULL) {
		t.debug = true;
	}

	char *disable_alpaca = getOption(argv, argv + argc, "--disable-alpaca");
	if(disable_alpaca != NULL) {
		t.disable_alpaca = true;
	}

	if(ticker != NULL) {
		t.ticker = ticker;
	}
	
	log.log("Starting bot.");
	
	t.trade();
}

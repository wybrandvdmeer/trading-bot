#include <iostream>
#include <algorithm>
#include <string>

#include "tradingbot.h"

using namespace std;

char const *USAGE="td\n";

char* getOption(char ** begin, char ** end, const std::string & option) {
    char ** itr = std::find(begin, end, option);
    if (itr != end && ++itr != end)
    {
        return *itr;
    }
    return NULL;
}

int main(int argc, char ** argv) {
	tradingbot tb("APPF");
	tb.configure();

	for(int idx=30; idx >= 0; idx--) {
		tb.trade(idx);
	}
	exit(0);
}

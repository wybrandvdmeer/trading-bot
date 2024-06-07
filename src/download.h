#include <string>
#include <ctime>

#include "logger.h"

#ifndef DOWNLOAD_H
#define DOWNLOAD_H

class download {
public:
	std::string request(std::string url);
	std::string request_bin_data(std::string url);
	download();
private:
	time_t cookie_time, crumb_time;
	std::string cookie, crumb;
	std::string get_crumb();
	void fetchCookie();
	std::string rq(std::string url, bool returnCookies=false);
	logger log;
};

#endif

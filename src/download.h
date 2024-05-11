#include <string>

#ifndef DOWNLOAD_H
#define DOWNLOAD_H

class download {
public:
	std::string request(std::string url);
	std::string request_bin_data(std::string url);
	download();
private:
	ulong cookieDate;
	std::string * cookie, crumb;
	std::string getCrumb();
	void fetchCookie();
	std::string rq(std::string url, bool returnCookies=false);
};

#endif

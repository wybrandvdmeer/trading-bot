#include <iostream>
#include <cstring>
#include <ctime>
#include <stdarg.h>

#include "logger.h"

using namespace std;

void logger::log(const char* fmt, ...) {
	char buf[1500];
	memset(buf, 0x00, 1000);
	va_list args;

	sprintf(buf, "%s", get_date().c_str());

	va_start(args, fmt);
	vsprintf(buf + 20, fmt, args);
	va_end(args);

	buf[strlen(buf)] = '\n';
	buf[strlen(buf) + 1] = 0x00;
	cout << buf << std::flush;
}

std::string logger::get_date() {
	time_t ts = time(0);
	struct tm *t = localtime(&ts);
	char buf[100];
	sprintf(buf, "%d-%02d-%02d %02d:%02d:%02d ", 
		t->tm_year + 1900, 
		t->tm_mon + 1, 
		t->tm_mday, 
		t->tm_hour, 
		t->tm_min, 
		t->tm_sec);
	return std::string(buf);
}

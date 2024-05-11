#include <iostream>
#include <cstring>
#include <stdarg.h>

#include "logger.h"

using namespace std;

void logger::log(const char* fmt, ...) {
	char buf[1000];
	memset(buf, 0x00, 1000);
	va_list args;
	va_start(args, fmt);
	vsprintf(buf, fmt, args);
	va_end(args);

	buf[strlen(buf)] = '\n';
	buf[strlen(buf) + 1] = 0x00;
	cout << buf;
}

#include <string>

#ifndef LOG_H
#define LOG_H

class logger {
public:
	inline static bool enable_date=true;
	void log(const char* fmt, ...);
private:
	std::string get_date();
};

#endif

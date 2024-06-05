#ifndef LOG_H
#define LOG_H

class logger {
public:
	void log(const char* fmt, ...);

private:
	std::string get_date();
};

#endif

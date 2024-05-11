#include <string>
#include <ctime>
#include <vector>

#include <sqlite3.h>

#include "candle.h"
#include "position.h"

#ifndef DB_API_H
#define DB_API_H

class db_api {
public:
	void createSchema();
	position * get_open_position(std::string ticker);
	void open_position(position p);
	void close_position(position p);
	void insert_candles(std::string ticker, std::vector<candle*> *candles);
private:
	sqlite3 * db;
	void open();
	void execDml(std::string sql);
	void close();
	void close(sqlite3_stmt * statement);
	float selectFloat(sqlite3_stmt * statement, int column);
	int selectInt(sqlite3_stmt * statement, int column);
	sqlite3_stmt * prepare(std::string sql);
	bool has_candle(std::string ticker, candle * c);
	void insert_candle(std::string ticker, candle *c);
};

#endif

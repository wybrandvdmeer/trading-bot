#include <string>
#include <ctime>
#include <vector>

#include <sqlite3.h>

#include "candle.h"
#include "macd.h"
#include "position.h"
#include "logger.h"
#include "indicators.h"

#ifndef DB_API_H
#define DB_API_H

class db_api {
public:
	bool debug;
	std::string ticker, strategy;
	db_api();

	void reset();
	void create_schema(int id);
	void drop_db();
	position * get_open_position(std::string ticker);
	void open_position(position p);
	void close_position(position p);
	void insert_candles(std::string ticker, std::vector<candle*> *candles, indicators ind);
	std::vector<candle*> *get_candles(std::string db_file);
	std::vector<position*> *get_closed_positions();
	float select_max_delta_close_sma_200();
	int select_max_candle_time();
	int get_owner_of_db_file(std::string db_file);
	bool lock_db(int id);
private:
	long max_candle_time;
	sqlite3 * db;
	void open_db();
	void open_db(std::string db_file);
	void open_db(std::string db_file, int timeout);
	void execDml(std::string sql);
	void execDml(std::string sql, bool ignore_error);
	void close_db();
	void close_db(sqlite3_stmt * statement);
	float selectFloat(sqlite3_stmt * statement, int column);
	std::string selectString(sqlite3_stmt * statement, int column);
	int selectInt(sqlite3_stmt * statement, int column);
	sqlite3_stmt * prepare(std::string sql);
	candle get_candle(std::string ticker, candle * c);
	void update_candle(std::string ticker, candle c);
	void insert_candle(std::string ticker, candle *c, float macd, float signal, float sma_50, 
		float sma_200);
	void get_date(std::string &s);
	std::string get_data_file();
	std::string get_data_file(bool uri);
	void update_indicators(candle *c, float ** custom_ind);
	int select_no_of_rows_of_table(std::string table);
	logger log;
	bool read_only;
};

#endif

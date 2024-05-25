#include <string>
#include <ctime>
#include <vector>

#include <sqlite3.h>

#include "candle.h"
#include "macd.h"
#include "position.h"
#include "logger.h"

#ifndef DB_API_H
#define DB_API_H

class db_api {
public:
	bool debug;
	std::string ticker;
	db_api();

	void reset();
	void create_schema();
	void drop_db();
	position * get_open_position(std::string ticker);
	void open_position(position p);
	void close_position(position p);
	void insert_candles(std::string ticker, std::vector<candle*> *candles, macd * m, 
		float sma_200, float * custom_ind1);
	std::vector<candle*> *get_candles(std::string db_file);
	std::vector<position*> *get_closed_positions();
	float select_max_delta_close_sma_200();
private:
	long max_candle_time;
	sqlite3 * db;
	void open();
	void open(std::string db_file);
	void execDml(std::string sql);
	void execDml(std::string sql, bool ignore_error);
	void close();
	void close(sqlite3_stmt * statement);
	float selectFloat(sqlite3_stmt * statement, int column);
	std::string selectString(sqlite3_stmt * statement, int column);
	int selectInt(sqlite3_stmt * statement, int column);
	sqlite3_stmt * prepare(std::string sql);
	bool has_candle(std::string ticker, candle * c);
	void insert_candle(std::string ticker, candle *c, float macd, float signal);
	void get_date(std::string &s);
	std::string get_data_file();
	std::string get_data_file(bool uri);
	void update_indicators(candle *c, float sma_200, float * custom_ind1);
	int select_no_of_rows_of_table(std::string table);
	logger log;
	bool read_only;
};

#endif

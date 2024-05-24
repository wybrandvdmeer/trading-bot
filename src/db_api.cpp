#include <iostream>
#include <string>
#include <ctime>
#include <vector>
#include <sqlite3.h>

#include "position.h"
#include "candle.h"
#include "db_api.h"
#include "macd.h"

using namespace std;

db_api::db_api() {
	debug = false;
	read_only = false;
	max_candle_time = 0L;
}

std::vector<position*> * db_api::get_closed_positions() {
	char sql[1000];
	std::vector<position*> *positions = new std::vector<position*>();

	read_only=true;
	open();
	read_only=false;

	sprintf(sql, "SELECT ticker, buy_time, sell_time, no_of_stocks, stock_price, sell_price \
		FROM positions WHERE sell_time IS NOT NULL ORDER BY id");

	if(debug) {
		log.log("%s", sql);
	}

	sqlite3_stmt * s = prepare(std::string(sql));

	for(;sqlite3_step(s) == SQLITE_ROW;) {
		position * p = new position();
		p->ticker = selectString(s, 0);
		p->buy = selectInt(s, 1);
		p->sell = selectInt(s, 2);
		p->no_of_stocks = selectInt(s, 3);
		p->stock_price = selectFloat(s, 4);
		p->sell_price = selectFloat(s, 5);
		positions->push_back(p);
	}

	return positions;
}
	
std::vector<candle*> * db_api::get_candles(std::string db_file) {
	char sql[1000];
	std::vector<candle*> *candles = new std::vector<candle*>();

	read_only=true;
	open(db_file);
	read_only=false;

	sprintf(sql, "SELECT time, open, close, low, high, volume FROM candles ORDER BY time");
	if(debug) {
		log.log("%s", sql);
	}

	sqlite3_stmt * s = prepare(std::string(sql));

	for(;sqlite3_step(s) == SQLITE_ROW;) {
		candle * c = new candle();
		c->time = selectInt(s, 0);
		c->open = selectFloat(s, 1);
		c->close = selectFloat(s, 2);
		c->low = selectFloat(s, 3);
		c->high = selectFloat(s, 4);
		c->volume = selectInt(s, 5);
		candles->push_back(c);
	}

	return candles;
}
	
void db_api::insert_candles(std::string ticker, std::vector<candle*> * candles, macd * m, 
	float sma_200) {	
	int idx=0;

	for(std::vector<candle*>::iterator it = candles->begin(); it != candles->end(); it++, idx++) {
		if((*it)->time > max_candle_time) {
			insert_candle(ticker, *it, 
				m->get_macd(candles->size() - 1 - idx), 
				m->get_signal(candles->size() - 1 - idx));
		
			db_api::max_candle_time = (*it)->time; 
		}
	}

	// sma value is only calculated for the last candle.
	update_sma_200(candles->at(candles->size() - 1), sma_200);
}

void db_api::update_sma_200(candle *c, float sma_200) {
	open();
	char sql[1000];
	sprintf(sql, "UPDATE candles SET sma_200 = %f WHERE time = %ld", sma_200, c->time);
	if(debug) {
		log.log("%s", sql);
	}

	execDml(sql);
}

void db_api::insert_candle(std::string ticker, candle *c, float macd, float signal) {	
	if(has_candle(ticker, c)) {
		return;
	}

	open();
	char sql[1000];
	sprintf(sql, "INSERT INTO candles(ticker, time, open, close, low, high, volume, macd, signal) \
		VALUES('%s', %ld, %.16f, %.16f, %.16f, %.16f, %ld, %f, %f)", 
		ticker.c_str(), 
		c->time, 
		c->open,
		c->close,
		c->low,
		c->high,
		c->volume, 
		macd,
		signal);
		 
	execDml(sql);
}

void db_api::close_position(position p) {
	open();
	char sql[1000];
	sprintf(sql, "UPDATE positions SET sell_time = %ld, sell_price = %f, stop_loss_activated = %d\
		WHERE id = %d", p.sell, p.sell_price, p.stop_loss_activated, p.id);
	execDml(sql);
}

void db_api::open_position(position p) {
	int no_of_positions = select_no_of_positions();
	open();
	char sql[1000];
	sprintf(sql, 
		"INSERT INTO POSITIONS(id, ticker, buy_time, no_of_stocks, stock_price, sell_off_price,\
		loss_limit_price, stop_loss_activated) VALUES(%d, '%s', %ld, %d, %f, %f, %f, 0)", 
		no_of_positions + 1,
		p.ticker.c_str(),
		p.buy,
		p.no_of_stocks,
		p.stock_price, 
		p.sell_off_price,
		p.loss_limit_price);

	execDml(sql);
}

float db_api::select_max_delta_close_sma_200() {
    open();
    char sql[1000];

    sprintf(sql, "SELECT MAX(close - sma_200) FROM candles WHERE sma_200 IS NOT NULL");

    if(debug) {
        log.log("%s", sql);
    }

    sqlite3_stmt * s = prepare(std::string(sql));
    sqlite3_step(s);

    if(sqlite3_data_count(s) == 0) {
        close(s);
        return 0;
    }

    float max_sma_200 = selectFloat(s, 0);

    close(s);
    return max_sma_200;
}

int db_api::select_no_of_positions() {
    open();
    char sql[1000];

    sprintf(sql, "SELECT COUNT(*) FROM positions");

    if(debug) {
        log.log("%s", sql);
    }

    sqlite3_stmt * s = prepare(std::string(sql));
    sqlite3_step(s);

    if(sqlite3_data_count(s) == 0) {
        close(s);
        return 0;
    }

    int count = selectInt(s, 0);

    close(s);
    return count;
}

bool db_api::has_candle(std::string ticker, candle *c) {
	open();
	char sql[1000];

	sprintf(sql, "SELECT COUNT(*) FROM candles WHERE ticker = '%s' AND time = %ld", 
		ticker.c_str(), c->time);

	if(debug) {
		log.log("%s", sql);
	}

	sqlite3_stmt * s = prepare(std::string(sql));
    sqlite3_step(s);

	if(sqlite3_data_count(s) == 0) {
		close(s);
		return false;
	}

	int count = selectInt(s, 0);

	close(s);
	return count != 0 ? true : false;
}

position * db_api::get_open_position(std::string ticker) {
	open();
	char sql[1000];
	sprintf(sql, 
	"SELECT id, buy_time, sell_time, no_of_stocks, stock_price, sell_off_price, loss_limit_price \
		FROM positions WHERE ticker = '%s' AND sell_time IS NULL",
		ticker.c_str());
	sqlite3_stmt * s = prepare(std::string(sql));
	sqlite3_step(s);

	if(sqlite3_data_count(s) == 0) {
		close(s);
		return NULL;
	}

	position *p = new position();
	p->ticker = ticker;
	p->id = selectInt(s, 0);
	p->buy = selectInt(s, 1);
	p->sell = selectInt(s, 2);
	p->no_of_stocks = selectInt(s, 3);
	p->stock_price = selectFloat(s, 4);
	p->sell_off_price = selectFloat(s, 5);
	p->loss_limit_price = selectFloat(s, 6);

	close(s);

	return p;
}

void db_api::get_date(std::string &s) {
	s.erase();

	time_t rawtime;
	struct tm * timeinfo;
	char buffer[80];

	time (&rawtime);
	timeinfo = localtime(&rawtime);
  
	strftime(buffer,sizeof(buffer),"%Y%m%d",timeinfo);
	s.append(buffer);
}

std::string db_api::get_data_file() {
	return get_data_file(true);
}

std::string db_api::get_data_file(bool uri) {
	std::string date_string;
	get_date(date_string);
	std::string data_file = "/db-files/" + db_api::ticker + "-" + date_string + ".db";
	if(!uri) {
		return data_file;
	}
	return "file:" + data_file;
}

void db_api::drop_db() {
	std::remove(get_data_file(false).c_str());
}

void db_api::open() {
	open(get_data_file());
}

void db_api::open(std::string db_file) {
	int ret;
	if(read_only) {
		ret = sqlite3_open_v2(db_file.c_str(), &db, SQLITE_OPEN_READONLY , NULL);
	} else {
		ret = sqlite3_open_v2(db_file.c_str(), &db,  SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE , NULL);
	}

	if(ret != SQLITE_OK) {
		printf("ERROR: can't open database: %s\n", sqlite3_errmsg(db));
		exit(1);
	}
}


void db_api::close() {
	close(NULL);
}

void db_api::close(sqlite3_stmt * statement) {
	if(statement != NULL) {
    	sqlite3_finalize(statement);
	}
	sqlite3_close(db);
}

void db_api::execDml(std::string sql) {
	execDml(sql, false);
}

void db_api::execDml(std::string sql, bool ignore_error) {
	if(debug) {
		log.log("%s", sql.c_str());
	}

	sqlite3_stmt* stmt;

    if(sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, NULL) != SQLITE_OK) {
        printf("ERROR: while compiling sql: %s\n", sqlite3_errmsg(db));
		if(!ignore_error) {
			exit(1);
		}
    }

    sqlite3_step(stmt);

end:    
	close(stmt);
}

void db_api::create_schema() {
	open();
	execDml("CREATE TABLE positions (id INTEGER NOT NULL, ticker VARCHAR(10), buy_time INTEGER , \
		sell_time INTEGER, no_of_stocks INTEGER, stock_price REAL, sell_price REAL, \
		sell_off_price REAL, loss_limit_price REAL, stop_loss_activated INTEGER)", true);
	open();
	execDml("CREATE TABLE candles (ticker VARCHAR(10), time INTEGER, open REAL, close REAL, low REAL, \
			 high REAL, volume INTEGER, macd REAL, signal REAL, sma_200 REAL)", true);
}

sqlite3_stmt * db_api::prepare(std::string sql) {
	sqlite3_stmt *statement;
	unsigned long ret=0;

	if(sqlite3_prepare(db, sql.c_str(), -1, &statement, NULL) != SQLITE_OK) {
        printf("ERROR: while preparing sql: %s\n", sqlite3_errmsg(db));
		exit(1);
	}
    
	return statement;
}

std::string db_api::selectString(sqlite3_stmt * statement, int column) {
	char * t = (char*)sqlite3_column_text(statement, column);
	return std::string(t);
}

int db_api::selectInt(sqlite3_stmt * statement, int column) {
	return sqlite3_column_int(statement, column);
}

float db_api::selectFloat(sqlite3_stmt * statement, int column) {
	return (float)sqlite3_column_double(statement, column);
}

void db_api::reset() {
	max_candle_time = 0L;
}

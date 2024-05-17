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
		insert_candle(ticker, *it, 
			m->get_macd(candles->size() - 1 - idx), 
			m->get_signal(candles->size() - 1 - idx));
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
		VALUES('%s', %ld, %f, %f, %f, %f, %ld, %f, %f)", 
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
	sprintf(sql, "UPDATE positions SET sell_time = %ld, stop_loss_activated = %d\
		WHERE ticker = '%s'", time(0), p.stop_loss_activated, p.ticker.c_str());
	execDml(sql);
}

void db_api::open_position(position p) {
	open();
	char sql[1000];
	sprintf(sql, 
		"INSERT INTO POSITIONS(ticker, buy_time, no_of_stocks, stock_price, sell_price,\
		loss_limit_price, stop_loss_activated) VALUES('%s', %ld, %d, %f, %f, %f, 0)", 
		p.ticker.c_str(),
		p.buy,
		p.no_of_stocks,
		p.stock_price, 
		p.sell_price,
		p.loss_limit_price);

	execDml(sql);
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
	"SELECT buy_time, sell_time, no_of_stocks, stock_price, sell_price, loss_limit_price \
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
	p->buy = selectInt(s, 0);
	p->sell = selectInt(s, 1);
	p->no_of_stocks = selectInt(s, 2);
	p->stock_price = selectFloat(s, 3);
	p->sell_price = selectFloat(s, 4);
	p->loss_limit_price = selectFloat(s, 5);

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
	std::string date_string;
	get_date(date_string);
	return "file:/db-files/" + db_api::ticker + "-" + date_string + ".db";
}

void db_api::drop_db() {
	std::remove(get_data_file().c_str());
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
	if(debug) {
		log.log("%s", sql.c_str());
	}

	sqlite3_stmt* stmt;

    if(sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, NULL) != SQLITE_OK) {
        printf("ERROR: while compiling sql: %s\n", sqlite3_errmsg(db));
		goto end;
    }

    sqlite3_step(stmt);

end:    
	close(stmt);
}

void db_api::create_schema() {
	open();
	execDml("CREATE TABLE positions (ticker VARCHAR(10), buy_time INTEGER , sell_time INTEGER, \
		no_of_stocks INTEGER, stock_price REAL, sell_price REAL, loss_limit_price REAL, \
		stop_loss_activated INTEGER)");

	open();
	execDml("CREATE TABLE candles (ticker VARCHAR(10), time INTEGER, open REAL, close REAL, low REAL, \
			 high REAL, volume INTEGER, macd REAL, signal REAL, sma_200 REAL)");
}

sqlite3_stmt * db_api::prepare(std::string sql) {
	sqlite3_stmt *statement;
	unsigned long ret=0;

	if(sqlite3_prepare(db, sql.c_str(), -1, &statement, NULL) != SQLITE_OK) {
		close(statement);
		return NULL;
	}
    
	return statement;
}

int db_api::selectInt(sqlite3_stmt * statement, int column) {
	return sqlite3_column_int(statement, column);
}

float db_api::selectFloat(sqlite3_stmt * statement, int column) {
	return (float)sqlite3_column_double(statement, column);
}

#include <iostream>
#include <string>
#include <ctime>
#include <vector>
#include <cstring>
#include <sqlite3.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "position.h"
#include "candle.h"
#include "db_api.h"
#include "macd.h"

using namespace std;

db_api::db_api() {
	debug = false;
	max_candle_time = 0L;
}

int db_api::get_owner_of_db_file(std::string db_file) {
	char sql[1000];

	open_db(db_file, 5000); // timeout, because function looks in db-files of other bots (tbl-lock).

	sqlite3_stmt *s = execute_select("SELECT value FROM config WHERE id = 'tb-id'");

	if(s == NULL) {
        close_db(s);
        return -1;
    }

	std::string id = selectString(s, 0);
	close_db(s);
	return std::stoi(id);
}

std::vector<position*> * db_api::get_closed_positions() {
	char sql[1000];
	std::vector<position*> *positions = new std::vector<position*>();

	open_db_ro();

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

	open_db_ro(db_file);

	sprintf(sql, "SELECT id, time, open, close, low, high, volume FROM candles ORDER BY time");
	if(debug) {
		log.log("%s", sql);
	}

	sqlite3_stmt * s = prepare(std::string(sql));

	for(;sqlite3_step(s) == SQLITE_ROW;) {
		candle * c = new candle();
		c->id = selectInt(s, 0);
		c->time = selectInt(s, 1);
		c->open = selectFloat(s, 2);
		c->close = selectFloat(s, 3);
		c->low = selectFloat(s, 4);
		c->high = selectFloat(s, 5);
		c->volume = selectInt(s, 6);
		candles->push_back(c);
	}

	return candles;
}
	
void db_api::insert_candles(std::string ticker, std::vector<candle*> * candles, indicators ind) {
	int idx=0;

	for(std::vector<candle*>::iterator it = candles->begin(); it != candles->end(); it++, idx++) {
		if((*it)->time > max_candle_time) {
			insert_candle(ticker, *it, 
				ind.m.get_macd(candles->size() - 1 - idx), 
				ind.m.get_signal(candles->size() - 1 - idx),
				ind.get_sma_50(candles->size() - 1 - idx),
				ind.get_sma_200(candles->size() - 1 - idx));
		
			db_api::max_candle_time = (*it)->time; 
		}
	}

	// sma value is only calculated for the last candle.
	update_indicators(candles->at(candles->size() - 1), ind.custom_ind);
}

void db_api::update_indicators(candle *c, float ** custom_ind) {
	char sql[1000];
	char sql2[100];
	*sql2 = 0x00;

	for(int idx=0; idx < 3; idx++) {
    	if(custom_ind[idx] != NULL) {
			sprintf(sql2 + strlen(sql2), "%s custom_ind%d = %f",
			idx != 0 ? "," : " ",
			idx + 1, *(custom_ind[idx]));
		} else {
			sprintf(sql2 + strlen(sql2), " ");
        }
	}
 
	if(strlen(sql2) == 3) {
		return;
	}

	open_db();
	sprintf(sql, "UPDATE candles SET %s WHERE time = %ld", 
		sql2,
		c->time);
	if(debug) {
		log.log("%s", sql);
	}

	execDml(sql);
}

void db_api::insert_candle(std::string ticker, candle *c, float macd, float signal, float sma_50,
	float sma_200) {	
	candle candle = get_candle(ticker, c);

	if(candle.time != 0) {
		/* Yahoo can correct their candles. If so, we have to update the candle.
		*/
		if(!candle.equals(*c)) {
			log.log("Update candle (%s,%ld)", ticker.c_str(), c->time);
			update_candle(ticker, *c);
		}
		return;
	}

	int no_of_candles = select_no_of_rows_of_table("candles");

	open_db();
	char sql[1000];
	sprintf(sql, "INSERT INTO candles(id, ticker, time, open, close, low, high, volume, macd, signal,\
 		sma_50, sma_200) VALUES(%d, '%s', %ld, %.16f, %.16f, %.16f, %.16f, %ld, %f, %f, %f, %f)",
		no_of_candles + 1,
		ticker.c_str(), 
		c->time, 
		c->open,
		c->close,
		c->low,
		c->high,
		c->volume, 
		macd,
		signal,
		sma_50,
		sma_200);
		 
	execDml(sql);
}

void db_api::close_position(position p) {
	open_db();
	char sql[1000];
	sprintf(sql, "UPDATE positions SET sell_time = %ld, sell_price = %f, stop_loss_activated = %d\
		WHERE id = %d", p.sell, p.sell_price, p.stop_loss_activated, p.id);
	execDml(sql);
}

void db_api::update_candle(std::string ticker, candle c) {
	open_db();
	char sql[1000];
	sprintf(sql, "UPDATE candles SET open = %.16f, close = %.16f, low = %.16f, high = %.16f, \
		volume = %ld WHERE time = %ld AND ticker = '%s'",
		c.open,
		c.close,
		c.low,
		c.high,
		c.volume,
		c.time,
		ticker.c_str());
	execDml(sql);
}

void db_api::open_position(position p) {
	int no_of_positions = select_no_of_rows_of_table("positions");
	open_db();
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
    open_db();

    sqlite3_stmt *s = execute_select(
		"SELECT MAX(close - sma_200) FROM candles WHERE sma_200 IS NOT NULL");
	
	if(s == NULL) {
    	return 0;
	}

    float max_sma_200 = selectFloat(s, 0);

    close_db(s);
    return max_sma_200;
}

int db_api::select_max_candle_time() {
    open_db();

    sqlite3_stmt *s = execute_select("SELECT MAX(time) FROM candles");
    if(s == NULL) { 
        close_db(s);
        return 0;
    }

    int max = selectInt(s, 0);

    close_db(s);
    return max;
}

int db_api::select_no_of_rows_of_table(std::string table) {
    open_db();

    sqlite3_stmt *s = execute_select("SELECT COUNT(*) FROM " + table);

    if(s == NULL) {
        close_db(s);
        return 0;
    }

    int count = selectInt(s, 0);

    close_db(s);
    return count;
}

candle db_api::get_candle(std::string ticker, candle *c) {
	open_db();
	char sql[1000];

	sprintf(sql, "SELECT open, close, low, high, volume \
		FROM candles WHERE ticker = '%s' AND time = %ld", 
		ticker.c_str(), c->time);

	sqlite3_stmt * s = execute_select(std::string(sql));
	if(s == NULL) {
		close_db(s);
		candle cc;
		return cc;
	}

	float open = selectFloat(s, 0);
	float closeP = selectFloat(s, 1);
	float low = selectFloat(s, 2);
	float high = selectFloat(s, 3);
	int volume = selectInt(s, 4);

	candle cc(c->time, open, closeP, low, high, volume);

	close_db(s);
	return cc;
}

position * db_api::get_open_position(std::string ticker) {
	open_db();
	char sql[1000];
	sprintf(sql, 
	"SELECT id, buy_time, sell_time, no_of_stocks, stock_price, sell_off_price, loss_limit_price \
		FROM positions WHERE ticker = '%s' AND sell_time IS NULL",
		ticker.c_str());

	sqlite3_stmt * s = execute_select(std::string(sql));
	if(s == NULL) {
		close_db(s);
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

	close_db(s);

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
	return get_data_file(false);
}

std::string db_api::get_data_file(bool uri) {
	std::string date_string;
	get_date(date_string);
	std::string data_file = "/db-files/" + db_api::ticker + "-" + date_string + "-" + strategy + ".db";
	if(!uri) {
		return data_file;
	}
	return "file:" + data_file;
}

void db_api::drop_db() {
	std::remove(get_data_file().c_str());
}

void db_api::open_db() {
	open_db(get_data_file(true));
}

void db_api::open_db(std::string db_file) {
	open_db(db_file, 0, false);
}

void db_api::open_db_ro() {
	open_db(get_data_file(), 0, true);
}

void db_api::open_db_ro(std::string db_file) {
	open_db(db_file, 0, true);
}

void db_api::open_db(std::string db_file, int timeout) {
	open_db(db_file, timeout, false);
}

void db_api::open_db(std::string db_file, int timeout, bool read_only) {
	int ret;
	if(read_only) {
		ret = sqlite3_open_v2(db_file.c_str(), &db, SQLITE_OPEN_READONLY , NULL);
	} else {
		ret = sqlite3_open_v2(db_file.c_str(), &db,  SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE , NULL);
	}

	if(timeout > 0) {
		sqlite3_busy_timeout(db, timeout);
	}

	if(ret != SQLITE_OK) {
		printf("ERROR: can't open database: %s\n", sqlite3_errmsg(db));
		exit(1);
	}
}

void db_api::close_db() {
	close_db(NULL);
}

void db_api::close_db(sqlite3_stmt * statement) {
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
	close_db(stmt);
}

void db_api::create_schema(int id) {
	open_db();
	execDml("CREATE TABLE config (id VARCHAR(50) PRIMARY KEY, value VARCHAR(50))", true);
	open_db();
	execDml("CREATE TABLE positions (id INTEGER NOT NULL, ticker VARCHAR(10), buy_time INTEGER , \
		sell_time INTEGER, no_of_stocks INTEGER, stock_price REAL, sell_price REAL, \
		sell_off_price REAL, loss_limit_price REAL, stop_loss_activated INTEGER)", true);
	open_db();
	execDml("CREATE TABLE candles (id INTEGER NOT NULL, ticker VARCHAR(10), time INTEGER, open REAL, \
			close REAL, low REAL, high REAL, volume INTEGER, macd REAL, signal REAL, sma_200 REAL, \
			sma_50 REAL, custom_ind1 REAL, custom_ind2 REAL, custom_ind3 REAL)", 
			true);
	
	char sql[1000];
	open_db();
	sprintf(sql, "INSERT INTO config VALUES('tb-id', %d)", id);
	execDml(sql);
}

sqlite3_stmt * db_api::prepare(std::string sql) {
	sqlite3_stmt *statement;
	unsigned long ret=0;

	if(sqlite3_prepare(db, sql.c_str(), -1, &statement, NULL) != SQLITE_OK) {
        printf("ERROR: while preparing sql: %s\n", sqlite3_errmsg(db));
		return NULL;
	}

	return statement;
}

bool db_api::lock_db(int id) {
	bool has_lock=false;
	std::string db_file = get_data_file();

	log.log("Try to lock db-file: %s.", db_file.c_str());

	int fd = open(db_file.c_str(), O_CREAT | O_EXCL, 0644);
	has_lock = (fd >= 0);
	
	log.log("Got lock is %d.", has_lock);

	if(!has_lock) {	
		close(fd);
		return false;
	}

	reset();

	create_schema(id);
	return true;
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

sqlite3_stmt * db_api::execute_select(std::string sql) {
    if(debug) {
        log.log("%s", sql);
    }

    sqlite3_stmt * s = prepare(std::string(sql));
    sqlite3_step(s);

    if(sqlite3_data_count(s) == 0) {
        close_db(s);
        return NULL;
    }
	return s;
}

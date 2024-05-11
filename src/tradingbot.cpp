#include <iostream>
#include <chrono>
#include <thread>
#include <ctime>

#include "candle.h"
#include "position.h"
#include "yahoo_api.h"
#include "db_api.h"
#include "logger.h"
#include "tradingbot.h"

using namespace std;

/* 
Gap & Go: identificeer een hogere opening tov de vorige dag en lift dan mee na bijv de 1e pull back.
Trendfollowing: Identificeer op dag basis een trend en stap dan in aan het begin vd trend op basis
van bijv de macd.
*/

tradingbot::tradingbot(std::string ticker) {
	tradingbot::ticker = ticker;
	tradingbot::smaSlowRange = 50;
	tradingbot::smaFastRange = 20;
}

void tradingbot::trade(int offset) {
	int noOfHistory = offset + 200 + 1; 
	std::vector<candle*> * candles = yahoo.stockPrices(ticker, "1d", std::to_string(noOfHistory) + "d");

	log.log("\nEvaluating %s", ticker.c_str());

	float sma200_0 = calculateSma(200, candles, 0 + offset);
	float smaSlow_0 = calculateSma(smaSlowRange, candles, 0 + offset);
	float smaFast_0 = calculateSma(smaFastRange, candles, 0 + offset);

	float ema12_0 = calculateEma(12, candles, 0 + offset);
	float ema26_0 = calculateEma(26, candles, 0 + offset);
	float macd_0 = ema12_0 - ema26_0;

	log.log("EMA12_0: %f", ema12_0);
	log.log("EMA26_0: %f", ema26_0);
	log.log("macd_0: %f", macd_0);

	float smaSlow_1 = calculateSma(smaSlowRange, candles, 1 + offset);
	float smaFast_1 = calculateSma(smaFastRange, candles, 1 + offset);

	float ema12_1 = calculateEma(12, candles, 1 + offset);
	float ema26_1 = calculateEma(26, candles, 1 + offset);
	float macd_1 = ema12_1 - ema26_1;

	log.log("EMA12_1: %f", ema12_1);
	log.log("EMA26_1: %f", ema26_1);
	log.log("macd_1: %f", macd_1);

	float open_0 = candles->at(candles->size() - 1 - offset)->open;
	float close_0 = candles->at(candles->size() - 1 - offset)->close;
	float close_1 = candles->at(candles->size() - 2 - offset)->close;

	log.log("Last open/close: (%f,%f)", open_0, close_0);
	
	position * p = db.get_open_position(ticker);
	if(p != NULL) {
		if(close_0 >= p->sell_price) {
			p->stop_loss_activated = 0;
			sell(p);
		}

		if(close_0 <= p->loss_limit_price) {
			p->stop_loss_activated = 1;
			sell(p);
		}
		return;
	}

	if(close_0 <= sma200_0) {
		log.log("no trade: price (%f) is below sma200 (%f).", close_0, sma200_0);
		return;
	}

	if(macd_0 < 0 && macd_1 < 0) {
		log.log("no trade: macd (%f,%f) is below zero.", macd_0, macd_1);
		return;
	}

	if(macd_0 < macd_1) {
		log.log("no trade: macd (%f,%f) has a negative trend.", macd_0, macd_1);
		return;
	}

	if(close_1 >= close_0) {
		log.log("no trade: close_0 is smaller then close_1 (%f,%f).", close_0, close_1);
		return;
	}

	buy(ticker, close_0, close_1);
}

void tradingbot::buy(std::string ticker, float stock_price, float loss_limit_price) {
	position p;
	p.ticker = ticker;
	p.buy = time(0);
	p.no_of_stocks = 1000 / ((int)stock_price);
	p.stock_price = stock_price;
	p.sell_price = stock_price + 10;
	p.loss_limit_price = loss_limit_price;
	
	log.log("Buy (%s), price: %f, number: %d, sell_price: %f, loss-limit: %f", 
		ticker.c_str(), 
		p.stock_price,
		p.no_of_stocks,
		p.sell_price,
		p.loss_limit_price);

	db.open_position(p);
}

void tradingbot::sell(position *p) {
	log.log("Sell (%s), price: %f, number: %d, sell_price: %f, loss-limit: %f, stop_loss_activated: %d", 
		ticker.c_str(), 
		p->stock_price,
		p->no_of_stocks,
		p->sell_price,
		p->loss_limit_price,
		p->stop_loss_activated);

	db.close_position(*p);
}

float tradingbot::calculateEma(int noOfDays, std::vector<candle*> * candles, int offset) {
	int idx=0;
	float ema=calculateSma(noOfDays, candles, offset);

	float alpha = 2/(noOfDays + 1);

	for(vector<candle*>::reverse_iterator it = candles->rbegin(); it != candles->rend(); ++it) {
		if(idx >= noOfDays + offset) {
			break;
		}
		
		candle * c = *it;

		ema = (c->close - ema) * alpha + ema;
	}

	return ema;
}

float tradingbot::calculateSma(int noOfDays, std::vector<candle*> * candles, int offset) {

	int idx=0;
	float sma=0;

	for(vector<candle*>::reverse_iterator it = candles->rbegin(); it != candles->rend(); ++it) {
		if(idx >= noOfDays + offset) {
			break;
		}

		candle * c = *it;

		if(c->close == 0) {
			continue;
		}

		if(idx >= offset) {
			sma += c->close;
		}

		idx++;
	}

	return sma /= noOfDays;
}



void tradingbot::configure() {
	db.createSchema();
}

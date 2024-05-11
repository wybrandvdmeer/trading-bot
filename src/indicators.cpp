#include <iostream>
#include <vector>

#include "candle.h"
#include "macd.h"
#include "indicators.h"
#include "logger.h"

using namespace std;

macd * indicators::calculate_macd(std::vector<candle*> * candles) {

	macd * m = new macd();

	m->ema_12 = calculate_ema(12, candles);
	m->ema_26 = calculate_ema(26, candles);

	float alpha = ((float)2)/10;

	float ema=0;

	for(int idx=0; idx < m->ema_12.size(); idx++) {
		float macd = m->ema_12.at(idx) - m->ema_26.at(idx);
		m->macd.push_back(macd);

		ema = macd * alpha + (1 - alpha) * ema;
		m->signal.push_back(ema);
	}

	return m;
}

float indicators::calculate_ema(int noOfDays, std::vector<candle*> * candles, int offset) {
	vector<float> emas = calculate_ema(noOfDays, candles);
	return emas.at(emas.size() - 1 - offset);
}

std::vector<float> indicators::calculate_ema(int noOfDays, std::vector<candle*> * candles) {
	float alpha = 2/((float)noOfDays + 1);
	float close, prv_close=0, ema = 0;

	vector<float> emas;

	vector<candle*>::iterator it;
	for(it = candles->begin(); it != candles->end(); it++) {
		candle * c = *it;

		close = c->close;
		if(close == 0) {
			close = prv_close;
		}

		ema = c->close * alpha + (1 - alpha) * ema;

		emas.push_back(ema);

		prv_close = close;
	}
	return emas;
}

float indicators::calculate_sma(int noOfDays, std::vector<candle*> * candles, int offset) {

	int idx=0;
	float prv_close=0, close, sma=0;

	for(vector<candle*>::reverse_iterator it = candles->rbegin(); it != candles->rend(); ++it) {
		if(idx >= noOfDays + offset) {
			break;
		}

		candle * c = *it;

		if(c->close == 0) {
			close = prv_close;
		} else {
			close = c->close;
		}

		if(idx >= offset) {
			sma += close;
		}

		idx++;

		prv_close = close;
	}

	return sma /= noOfDays;
}

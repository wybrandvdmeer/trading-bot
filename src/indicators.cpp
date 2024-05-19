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

		/* If corresponding candle is not valid, then we skip its signal value.
		*/
		if(!candles->at(idx)->is_valid()) {
			m->signal.push_back(ema);
			continue;
		}

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
	float ema = 0;

	vector<float> emas;

	vector<candle*>::iterator it;
	for(it = candles->begin(); it != candles->end(); it++) {
		candle * c = *it;

		if(!c->is_valid()) {
			/* To keep the macd vector the same size as the candle vector ie for every candle
			there is a macd which is non valid. 
			*/
			emas.push_back(emas.size() > 0 ? emas.at(emas.size() - 1) : 0);
			continue;
		}

		ema = c->close * alpha + (1 - alpha) * ema;

		emas.push_back(ema);
	}
	return emas;
}

float indicators::calculate_sma(int noOfDays, std::vector<candle*> * candles, int offset) {
	int idx=0;
	float close, sma=0;

	for(vector<candle*>::reverse_iterator it = candles->rbegin(); it != candles->rend(); ++it) {
		if(idx >= noOfDays + offset) {
			break;
		}

		candle * c = *it;

		if(!c->is_valid()) {
			continue;
		} 
	
		if(idx >= offset) {
			sma += c->close;
		}

		idx++;
	}

	return sma /= noOfDays;
}

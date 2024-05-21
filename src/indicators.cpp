#include <iostream>
#include <vector>

#include "candle.h"
#include "macd.h"
#include "indicators.h"
#include "logger.h"

using namespace std;

void indicators::calculate_macd(std::vector<float> prices) {

	/* The size of the price windows has a max. 
	*/
	if(prices.size() <= m.ema_12.size()) {
		int end = m.ema_12.size() - prices.size() + 1;
		log.log("Erasing macd elements in range: 0 - %d.", end);

		m.ema_12.erase(m.ema_12.begin(), m.ema_12.begin() + end);
		m.ema_26.erase(m.ema_26.begin(), m.ema_26.begin() + end);
		m.macd.erase(m.macd.begin(), m.macd.begin() + end);
		m.signal.erase(m.signal.begin(), m.signal.begin() + end);
	}

	calculate_ema(12, prices, &m.ema_12);
	calculate_ema(26, prices, &m.ema_26);

	float alpha = ((float)2)/10;

	float ema=0;

	for(int idx=m.macd.size(); idx < m.ema_12.size(); idx++) {
		float macd = m.ema_12.at(idx) - m.ema_26.at(idx);
		m.macd.push_back(macd);
	}

	calculate_ema(9, m.macd, &m.signal);
}

float indicators::calculate_ema(int no_of_days, std::vector<float> prices) {
	std::vector<float> emas;
	calculate_ema(no_of_days, prices, &emas);
	return emas.at(emas.size() - 1);
}

void indicators::calculate_ema(int no_of_days, std::vector<float> prices, std::vector<float> * emas) {
	float alpha = 2/((float)no_of_days + 1);
	float ema = emas->size() > 0 ? emas->at(emas->size() - 1) : 0;

	int idx=0;
	for(vector<float>::iterator it = prices.begin(); it != prices.end(); it++, idx++) {
		if(idx < emas->size()) {
			continue;
		}

		ema = *it * alpha + (1 - alpha) * ema;

		emas->push_back(ema);
	}
}

float indicators::calculate_sma(int no_of_days, std::vector<float> prices) {
	int idx=0;
	float sma=0;

	for(vector<float>::reverse_iterator it = prices.rbegin(); it != prices.rend(); it++) {
		if(idx >= no_of_days) {
			break;
		}

		sma += (*it);

		idx++;
	}

	return sma /= no_of_days;
}

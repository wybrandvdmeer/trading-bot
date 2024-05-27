#include <iostream>
#include <vector>

#include "candle.h"
#include "macd.h"
#include "indicators.h"
#include "logger.h"

using namespace std;

void indicators::reset(std::string ticker) {
	if(indicators::ticker != ticker) {
		log.log("Clearing indicators.");
		sma_50.clear();
		sma_200.clear();
		m.ema_12.clear();
		m.ema_26.clear();
		m.macd.clear();
		m.signal.clear();
		indicators::ticker = ticker;
	}
}

void indicators::calculate_macd(std::vector<float> prices) {
	calculate_ema(12, prices, &m.ema_12);
	calculate_ema(26, prices, &m.ema_26);

	float alpha = ((float)2)/10;

	float ema=0;

	for(int idx=m.macd.size(); idx < m.ema_12.size(); idx++) {
		m.macd.push_back(m.ema_12.at(idx) - m.ema_26.at(idx));
	}

	calculate_ema(9, m.macd, &m.signal);
}

void indicators::calculate_sma_200(std::vector<float> prices) {
	calculate_sma(200, prices, &sma_200);
}

void indicators::calculate_sma_50(std::vector<float> prices) {
	calculate_sma(50, prices, &sma_50);
}

float indicators::calculate_ema(int no_of_days, std::vector<float> prices) {
	std::vector<float> emas;
	calculate_ema(no_of_days, prices, &emas);
	return emas.at(emas.size() - 1);
}

void indicators::calculate_ema(int no_of_days, std::vector<float> prices, std::vector<float> * emas) {
	float alpha = 2/((float)no_of_days + 1);
	float ema = emas->size() > 0 ? emas->at(emas->size() - 1) : prices.at(0);

	int idx=0;
	for(vector<float>::iterator it = prices.begin(); it != prices.end(); it++, idx++) {
		if(idx < emas->size()) {
			continue;
		}

		ema = *it * alpha + (1 - alpha) * ema;

		emas->push_back(ema);
	}
}

void indicators::calculate_sma(int no_of_days, std::vector<float> prices, std::vector<float> *smas) {
	int day=1;
	for(vector<float>::iterator it = prices.begin(); it != prices.end(); it++, day++) {
		if(day < smas->size()) {
			continue;
		}

		int begin = day - no_of_days >= 0 ? day - no_of_days : 0;
		float sma=0;
		int idx=begin;
		for(vector<float>::iterator it = prices.begin() + begin; 
			idx < day && it != prices.end(); it++, idx++) {
			sma += (*it);
		}
		
		smas->push_back(sma/(day < no_of_days ? day : no_of_days));
	}
}

float indicators::get_sma_200(int offset) {
	return sma_200.at(sma_200.size() - offset - 1); 
}

float indicators::get_sma_50(int offset) {
	return sma_50.at(sma_50.size() - offset - 1);
}

float indicators:: get_sma_diff_50_200(int offset) {
	return get_sma_50(offset) - get_sma_200(offset);
}

bool indicators::is_sma_50_200_diff_trending(int length, bool positive_trend) {
	for(int idx=0; idx < length && idx < sma_50.size() - 1; idx++) {
		if(positive_trend && get_sma_diff_50_200(idx) < get_sma_diff_50_200(idx + 1)) {
			return false;
		}
		if(!positive_trend && get_sma_diff_50_200(idx) > get_sma_diff_50_200(idx + 1)) {
			return false;
		} 
	}

	return true;
}


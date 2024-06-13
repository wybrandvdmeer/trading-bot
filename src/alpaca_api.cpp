#include "alpaca/client.h"
#include "alpaca/config.h"

#include "position.h"
#include "logger.h"

#include "alpaca_api.h"

bool alpaca_api::open_position(position &p) {	
	auto env = alpaca::Environment();
	auto client = alpaca::Client(env);

	alpaca::StopLossParams stop_loss_params;
	stop_loss_params.stopPrice = std::to_string(p.loss_limit_price);

	auto response = client.submitOrder(
  		p.ticker,
  		p.no_of_stocks,
  		alpaca::OrderSide::Buy,
  		alpaca::OrderType::Market,
  		alpaca::OrderTimeInForce::Day,
		"",
		"",
		false,
		"", 		// order id.
		alpaca::OrderClass::Simple,
		NULL,
		&stop_loss_params
	);

	if (auto status = response.first; !status.ok()) {
		log.log("Error calling API: %s", status.getMessage().c_str());
		return false; 
	}


	return true;
}

bool alpaca_api::close_position(position &p) {	
	auto env = alpaca::Environment();
	auto client = alpaca::Client(env);

	auto response = client.closePosition(p.ticker);
	if (auto status = response.first; !status.ok()) {
		log.log("Error calling API: %s", status.getMessage().c_str());
		return false; 
	}

	return true;
}

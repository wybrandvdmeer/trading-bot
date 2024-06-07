EXE=tb

FLAGS = -std=c++20

LIBS = -lcurl -lsqlite3 -lalpaca_client

$(EXE): main.o download.o yahoo_api.o candle.o tradingbot.o db_api.o logger.o indicators.o macd.o \
		top_gainers.o alpaca_api.o strategy.o macd_scavenging_strategy.o sma_crossover_strategy.o
	g++ -Wall -o $(EXE) download.o yahoo_api.o candle.o db_api.o logger.o \
		indicators.o macd.o top_gainers.o alpaca_api.o strategy.o sma_crossover_strategy.o \
		macd_scavenging_strategy.o tradingbot.o main.o $(LIBS)

main.o: main.cpp
	g++ $(FLAGS) -c main.cpp

strategy.o: strategy.cpp strategy.h
	g++ $(FLAGS) -c strategy.cpp

sma_crossover_strategy.o: sma_crossover_strategy.cpp strategy.h strategy.cpp
	g++ $(FLAGS) -c sma_crossover_strategy.cpp

macd_scavenging_strategy.o: macd_scavenging_strategy.cpp strategy.h strategy.cpp
	g++ $(FLAGS) -c macd_scavenging_strategy.cpp

alpaca_api.o: alpaca_api.cpp alpaca_api.h
	g++ $(FLAGS) -c alpaca_api.cpp

download.o: download.cpp download.h
	g++ $(FLAGS) -c download.cpp

top_gainers.o: top_gainers.cpp top_gainers.h
	g++ $(FLAGS) -c top_gainers.cpp

macd.o: macd.cpp macd.h
	g++ $(FLAGS) -c macd.cpp

logger.o: logger.cpp logger.h
	g++ $(FLAGS) -c logger.cpp

indicators.o: indicators.cpp indicators.h macd.h
	g++ $(FLAGS) -c indicators.cpp

db_api.o: db_api.cpp db_api.h position.h
	g++ $(FLAGS) -c db_api.cpp

tradingbot.o: tradingbot.cpp tradingbot.h position.h
	g++ $(FLAGS) -c tradingbot.cpp

candle.o: candle.cpp candle.h
	g++ $(FLAGS) -c candle.cpp

yahoo_api.o: yahoo_api.cpp yahoo_api.h stock.h
	g++ $(FLAGS) -c yahoo_api.cpp

install_py:
	sudo cp get_candles.py /usr/bin/get_candles
	sudo cp calculate_gain.py /usr/bin/calculate_gain
	sudo chmod 644 /usr/bin/get_candles
	sudo chmod +x /usr/bin/get_candles
	sudo chmod 644 /usr/bin/calculate_gain
	sudo chmod +x /usr/bin/calculate_gain

install: clean $(EXE) 
	sudo cp tb /usr/bin

clean:
	rm -rf *.o $(EXE)

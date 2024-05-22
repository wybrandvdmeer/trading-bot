EXE=tb

LIBS = -lcurl -lsqlite3 -lalpaca_client

$(EXE): main.o download.o yahoo_api.o candle.o tradingbot.o db_api.o logger.o indicators.o macd.o \
		top_gainers.o alpaca_api.o
	g++ -Wall -o $(EXE) main.o download.o yahoo_api.o candle.o db_api.o tradingbot.o logger.o \
		indicators.o macd.o top_gainers.o alpaca_api.o $(LIBS)

main.o: main.cpp
	g++ -c main.cpp

alpaca_api.o: alpaca_api.cpp alpaca_api.h
	g++ -c alpaca_api.cpp

download.o: download.cpp download.h
	g++ -c download.cpp

top_gainers.o: top_gainers.cpp top_gainers.h
	g++ -c top_gainers.cpp

macd.o: macd.cpp macd.h
	g++ -c macd.cpp

logger.o: logger.cpp logger.h
	g++ -c logger.cpp

indicators.o: indicators.cpp indicators.h macd.h
	g++ -c indicators.cpp

db_api.o: db_api.cpp db_api.h position.h
	g++ -c db_api.cpp

tradingbot.o: tradingbot.cpp tradingbot.h position.h
	g++ -c tradingbot.cpp

candle.o: candle.cpp candle.h
	g++ -c candle.cpp

yahoo_api.o: yahoo_api.cpp yahoo_api.h stock.h
	g++ -c yahoo_api.cpp

install: 
	sudo cp tb /usr/bin

clean:
	rm -rf *.o $(EXE)

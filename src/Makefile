EXE=tb

LIBS = -lcurl -lsqlite3

$(EXE): main.o download.o yahoo_api.o candle.o tradingbot.o db_api.o logger.o
	g++ -Wall -o $(EXE) main.o download.o yahoo_api.o candle.o db_api.o tradingbot.o logger.o $(LIBS)

main.o: main.cpp
	g++ -c main.cpp

download.o: download.cpp download.h
	g++ -c download.cpp

logger.o: logger.cpp logger.h
	g++ -c logger.cpp

db_api.o: db_api.cpp db_api.h position.h
	g++ -c db_api.cpp

tradingbot.o: tradingbot.cpp tradingbot.h position.h
	g++ -c tradingbot.cpp

candle.o: candle.cpp candle.h
	g++ -c candle.cpp

yahoo_api.o: yahoo_api.cpp yahoo_api.h stock.h
	g++ -c yahoo_api.cpp

clean:
	rm -rf *.o $(EXE)

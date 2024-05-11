LIB=libalpaca_client.so

LIBS = -lssl -lglog -lcpp-httplib
FLAGS = -DCPPHTTPLIB_OPENSSL_SUPPORT -fPIC

$(LIB): account.o asset.o bars.o calendar.o client.o clock.o config.o order.o portfolio.o position.o quote.o status.o trade.o watchlist.o
	g++ -Wall -shared -o $(LIB) account.o asset.o bars.o calendar.o client.o clock.o config.o order.o portfolio.o position.o quote.o status.o trade.o watchlist.o $(LIBS)

account.o: account.cpp
	g++ -c account.cpp $(FLAGS)

asset.o: asset.cpp
	g++ -c asset.cpp $(FLAGS)

bars.o: bars.cpp
	g++ -c bars.cpp $(FLAGS)

calendar.o: calendar.cpp
	g++ -c calendar.cpp $(FLAGS)

client.o: client.cpp
	g++ -c client.cpp $(FLAGS)

clock.o: clock.cpp
	g++ -c clock.cpp $(FLAGS)

config.o: config.cpp
	g++ -c config.cpp $(FLAGS)

order.o: order.cpp
	g++ -c order.cpp $(FLAGS)

portfolio.o: portfolio.cpp
	g++ -c portfolio.cpp $(FLAGS)

position.o: position.cpp
	g++ -c position.cpp $(FLAGS)

quote.o: quote.cpp
	g++ -c quote.cpp $(FLAGS)

status.o: status.cpp
	g++ -c status.cpp $(FLAGS)

streaming.o: streaming.cpp
	g++ -c streaming.cpp $(FLAGS)

trade.o: trade.cpp
	g++ -c trade.cpp $(FLAGS)

watchlist.o: watchlist.cpp
	g++ -c watchlist.cpp $(FLAGS)

clean:
	rm -rf *.o *.so

install:
	sudo cp $(LIB) /usr/lib/x86_64-linux-gnu

all: serverlib serverexecuable clientlib testclient libcs
	
serverlib:
	g++ -c ./src/smslserver.cpp

serverexecuable:
	g++ -o ./run_server ./src/run_server.cpp ./smslserver.o -lpthread

clientlib:
	g++ -c ./src/smslclient.cpp
    
testclient:
	g++ -o ./test_client ./src/test_client_interface.cpp ./smslclient.o -lpthread

libcs:
	mkdir ./lib
	ar rcs ./lib/libsmslcs.a ./smslclient.o ./smslserver.o
	rm -f ./smslclient.o ./smslserver.o

install:
	cp ./lib/libsmslcs.a /usr/local/lib
	cp ./include/* /usr/local/include

clean:
	rm -rf ./lib
	rm -f ./run_server ./test_client

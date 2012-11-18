server_1: connection_manager.cc utils.cc kvs_server_1.cc
	g++ -o connection_manager.o -c connection_manager.cc
	g++ -o utils.o -c utils.cc
	g++ -o kvs_server_1.o -c kvs_server_1.cc
	g++ -o server_1 connection_manager.o utils.o kvs_server_1.o -lpthread

client_1: client_1.cc
	g++ -o client_1 client_1.cc -lpthread

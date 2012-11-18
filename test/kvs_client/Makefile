
client_1: client_1.cc kvs_client.cc utils.cc
	g++ -o client_1.o -c client_1.cc
	g++ -o kvs_client.o -c kvs_client.cc
	g++ -o utils.o -c utils.cc
	g++ -o client_1 client_1.o kvs_client.o utils.o -lpthread

server_1: server_1.cc
	g++ -o server_1 server_1.cc

client_2: client_2.cc kvs_client.cc utils.cc
	g++ -o client_2.o -c client_2.cc
	g++ -o kvs_client.o -c kvs_client.cc
	g++ -o utils.o -c utils.cc
	g++ -o client_2 client_2.o kvs_client.o utils.o -lpthread

server_2: server_2.cc
	g++ -o server_2 server_2.cc

client_3: client_3.cc kvs_client.cc utils.cc
	g++ -o client_3.o -c client_3.cc
	g++ -o kvs_client.o -c kvs_client.cc
	g++ -o utils.o -c utils.cc
	g++ -o client_3 client_3.o kvs_client.o utils.o -lpthread

server_3: server_3.cc
	g++ -o server_3 server_3.cc -lpthread

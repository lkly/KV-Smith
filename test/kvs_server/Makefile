server_1: server_main_1.cc connection_manager.cc tiny_table.cc kvs_server.cc utils.cc replicated_log_1.cc
	g++ -o connection_manager.o -c connection_manager.cc
	g++ -o tiny_table.o -c tiny_table.cc
	g++ -o kvs_server.o -c kvs_server.cc
	g++ -o utils.o -c utils.cc
	g++ -o server_main_1.o -c server_main_1.cc
	g++ -o replicated_log_1.o -c replicated_log_1.cc
	g++ -o server_1 server_main_1.o connection_manager.o tiny_table.o kvs_server.o utils.o replicated_log_1.o -lpthread

server_2: server_main_2.cc connection_manager.cc tiny_table.cc kvs_server.cc utils.cc replicated_log_2.cc
	g++ -o connection_manager.o -c connection_manager.cc
	g++ -o tiny_table.o -c tiny_table.cc
	g++ -o kvs_server.o -c kvs_server.cc
	g++ -o utils.o -c utils.cc
	g++ -o server_main_2.o -c server_main_2.cc
	g++ -o replicated_log_2.o -c replicated_log_2.cc
	g++ -o server_2 server_main_2.o connection_manager.o tiny_table.o kvs_server.o utils.o replicated_log_2.o -lpthread


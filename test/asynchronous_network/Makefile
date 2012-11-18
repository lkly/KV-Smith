client_1: client_1.cc
	g++ -o client_1 client_1.cc -lpthread

server_1: paxos_1.cc asynchronous_network.cc utils.cc
	g++ -o utils.o -c utils.cc
	g++ -o paxos_1.o -c paxos_1.cc
	g++ -o asynchronous_network.o -c asynchronous_network.cc
	g++ -o server_1 paxos_1.o asynchronous_network.o utils.o -lpthread

s_pl: s_pl.cc utils.cc paxos_log.cc
	g++ -o s_pl s_pl.cc utils.cc paxos_log.cc

s_pl_2: s_pl_2.cc utils.cc paxos_log.cc
	g++ -o s_pl_2 s_pl_2.cc utils.cc paxos_log.cc


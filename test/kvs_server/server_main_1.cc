#include "kvs_server.h"

int servers_size = 3;

const char *servers[] = {
	"A 127.0.0.1 20000",
	"B 127.0.0.1 20001",
	"C 127.0.0.1 20002",
};

const char *addr = "127.0.0.1 10000";

void
cons_table(map<server_name, server_address> &table) {
	int i = 0;
	for (; i < servers_size; i++) {
		stringstream ss;
		ss << servers[i];
		string name;
		string ip_addr;
		string port;
		ss >> name;
		ss >> ip_addr;
		ss >> port;
		table[name] = make_pair(ip_addr, port);
	}
}

void
cons_addr(server_address &address) {
	stringstream ss;
	ss << addr;
	string ip_addr;
	ss >> ip_addr;
	string port;
	ss >> port;
	address.first = ip_addr;
	address.second = port;
}

int
main() {
	map<server_name, server_address> table;
	cons_table(table);
	server_address address;
	cons_addr(address);
	string name;
	name = "A";
	kvs_server *server = new kvs_server(name, address, table);
	//wait connection manager started.
	sleep(2);
	//shouldn't return.
	server->start();
	return 0;
}




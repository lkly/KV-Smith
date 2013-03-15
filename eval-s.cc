#include "kvs_server.h"
#include <stdio>
#include <fstream>



const char *servers[] = {
	"A 127.0.0.1 20000",
	"B 127.0.0.1 20001",
	"C 127.0.0.1 20002",
};

void
cons_table(map<server_name, server_address> &table) {
	fstream fs("kvs-s.config", ios::in);
	string line;
	stringstream ss;
	while (!fs.eof()) {
		getline(fs, line);
		if (line.length() == 0) {
			continue;
		}
		ss << line;
		string name;
		string ip_addr;
		string port;
		ss >> name;
		ss >> ip_addr;
		ss >> port;
		table[name] = make_pair(ip_addr, port);
	}
	fs.close();
}

int
main(int argc, char *argv[]) {
	//eval-s name delay
	if (argc != 3) {
		cout << "wrong number of arguments." << endl;
		exit(1);
	}
	stringstream ss;
	ss << argv[1];
	string name;
	ss >> name;
	ss << argv[2];
	int delay;
	ss >> delay;
	map<server_name, server_address> table;
	cons_table(table);
	kvs_server *server = new kvs_server(name, table[name], table);
	//wait connection manager started.
	//and mastership established.
	sleep(2+delay);
	//shouldn't return.
	server->start();
	return 0;
}


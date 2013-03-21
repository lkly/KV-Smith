#include "kvs_server.h"
#include <stdio>
#include <fstream>


void
cons_table(srting &name, map<server_name, server_address> &table, map<server_name, server_address> &table2) {
	stringstream fns;
	fn << "kvs-s-";
	fn << name;
	fn << ".config";
	string fn;
	fns >> fn;
	fstream fs(fn.c_str(), ios::in);
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
		ss >> ip_addr;
		ss >> port;
		table2[name] = make_pair(ip_addr, port);
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
	map<server_name, server_address> table2;
	cons_table(name, table, table2);
	kvs_server *server = new kvs_server(name, table2[name], table);
	//wait connection manager started.
	//and mastership established.
	sleep(2+delay);
	//shouldn't return.
	server->start();
	return 0;
}


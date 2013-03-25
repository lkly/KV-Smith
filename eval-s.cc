#include "kvs_server.h"
#include <iostream>
#include <fstream>
#include <stdlib.h>

void
cons_table(string &myname, map<server_name, server_address> &table, map<server_name, server_address> &table2) {
	stringstream fns;
	fns << "kvs-s-";
	fns << myname;
	fns << ".config";
	string fn;
	fns >> fn;
	fstream fs(fn.c_str(), ios::in);
	string line;

	while (!fs.eof()) {
		getline(fs, line);
		if (line.length() == 0) {
			continue;
		}
		cout << line << endl;
		stringstream ss;
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

	string name;
	name = argv[1];
	int delay;
	delay = atoi(argv[2]);

	map<server_name, server_address> table;
	map<server_name, server_address> table2;
	cons_table(name, table, table2);
	cout << table2[name].first << ":" << table2[name].second << endl;
	kvs_server *server = new kvs_server(name, table2[name], table);
	//wait connection manager started.
	//and mastership established.

	cout << "" << endl;
	sleep(2+delay);
	cout << "start" << endl;

	//shouldn't return.
	server->start();
	return 0;
}


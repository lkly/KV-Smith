#include "replicated_log.h"
#include <iostream>

using namespace std;


const char *servers[] = {
	"A 127.0.0.1 20000",
	"B 127.0.0.1 20001",
	"C 127.0.0.1 20002",
};

map<server_name, server_address> table;

void
cons_table(map<server_name, server_address> &table) {
	int i = 0;
	for (; i < 3; i++) {
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
test(replicated_log *rl, string name) {
	
}

int main(int argc, char *argv[]) {
	if (argc != 2) {
		cout << "REQUIRE 2 args" << endl;
		exit(1);
	}
	cons_table(table);
	string name = argv[1];
	if (name != "A" && name != "B" && name != "C") {
		cout << "BAD name: " << name << endl;
		exit(1);
	}
	replicated_log *rl = new replicated_log(name, table);
	//wait servers up.
	sleep(3);
	test(rl, name);	
	cout << name << ": TEST OK" << endl;
	return 0;
}



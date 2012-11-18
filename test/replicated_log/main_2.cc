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

int rds_size = 10;
const char *rds[] = {
	"first_record",
	"second_record",
	"third_record",
	"fourth_record",
	"fifth_record",
	"sixth_record",
	"seventh_record",
	"eighth_record",
	"ninth_record",
	"tenth_record",
};

void
test(string name, string p_name, replicated_log *rl) {
	if (name == p_name) {
		int i = 0;
		replicated_log::status r;
		for (; i < rds_size; i++) {
			string record = rds[i];
			r = rl->log(record, true, 2);
			if (r != replicated_log::OK) {
				cout << "WRONG status: " << "(" << i << " " << r << ")"<< endl;
				exit(1);
			}
		}
		string dull;
		if (rl->next_record(dull, 1)) {
			cout << "UNKNOWN record." << endl;
			exit(1);
		}
		return;
	}
	//wait priamry send out packets.
	//no next_record timer in tests.
	sleep(2);
	int i = 0;
	for (; i < rds_size; i++) {
		string record;
		bool r;
		r = rl->next_record(record, 1);
		if (!r) {
			cout << "DON'T get record " << i << endl;
			exit(1);
		}
		if (record != rds[i]) {
			cout << "UNMATCHED record " << i << ": " << record << " " << rds[i] << endl;
			exit(1);
		}
	}
	string dull;
	if (rl->next_record(dull, 1)) {
		cout << "UNKNOWN record." << endl;
		exit(1);
	}
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
	test(name, "A", rl);
	cout << name << ": TEST OK" << endl;
	return 0;
}


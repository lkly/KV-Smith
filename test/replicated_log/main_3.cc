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

const char *rds[] = {
	"A_1_RECORD",
	"A_2_RECORD",
	"B_1_RECORD",
	"A_3_RECORD",
};

void
good_log(string name, replicated_log *rl, string record, bool stable) {
	replicated_log::status r;
	r = rl->log(record, stable, 2);
	if (r != replicated_log::OK) {
		cout << name << " WRONG status: " << r << endl;
		exit(1);
	}
	cout << name << " GOOD_LOG: " << record << endl;
}

void
failed_log(string name, replicated_log *rl, string record, bool stable) {
	replicated_log::status r;
	r = rl->log(record, stable, 2);
	if (r != replicated_log::FAIL) {
		cout << name << " WRONG status: " << r << endl;
		exit(1);
	}
	cout << name << " FAILED_LOG: " << record << endl;
}

void
to_log(string name, replicated_log *rl, string record, bool stable) {
	replicated_log::status r;
	r = rl->log(record, stable, 2);
	if (r != replicated_log::TIMEOUT) {
		cout << name << " WRONG status: " << r << endl;
		exit(1);
	}
	cout << name << " TO_LOG: " << record << endl;
}

void
good_read(replicated_log *rl, string record) {
	string r;
	if (!rl->next_record(r, 1)) {
		cout << "FAIL to read." << endl;
		exit(1);
	}
	if (r != record) {
		cout << "UNMATCHING record: " << r << ":" << record << endl;
		exit(1);
	}
	cout << "GOOD READ: " << record << endl;
}

void
test(replicated_log *rl, string name) {
	if (name == "A") {
		good_log(name, rl, rds[0], false);
		good_log(name, rl, rds[1], true);
		sleep(3);
		to_log(name, rl, rds[2], true);
		//try log again.
		failed_log(name, rl, rds[2], true);
		rl->reset();
		good_read(rl, rds[2]);
		good_log(name, rl, rds[3], false);
		sleep(2);
	}
	if (name == "B") {
		sleep(1);
		to_log(name, rl, rds[0], true);
		rl->reset();
		good_read(rl, rds[0]);
		good_read(rl, rds[1]);
		good_log(name, rl, rds[2], false);
		sleep(3);
		to_log(name, rl, rds[3], true);
		rl->reset();
		good_read(rl, rds[3]);
	}
	if (name == "C") {
		sleep(5);
		to_log(name, rl, rds[0], true);
		rl->reset();
		good_read(rl, rds[0]);
		good_read(rl, rds[1]);
		good_read(rl, rds[2]);
		failed_log(name, rl, rds[3], false);
		rl->reset();
		good_read(rl, rds[3]);
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
	test(rl, name);	
	cout << name << ": TEST OK" << endl;
	return 0;
}



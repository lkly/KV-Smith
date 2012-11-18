#include "kvs_client.h"
#include <map>
#include <string>
#include <sstream>
#include <iostream>

using namespace std;

//this should reflect the size of the array servers correctly.
int servers_size = 1;

const char *servers[] = {
	"A 127.0.0.1 10000",
};

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
print_table(map<server_name, server_address> &table) {
	map<server_name, server_address>::iterator it;
	for (it = table.begin(); it != table.end(); it++) {
		cout << it->first << ": ";
		cout << it->second.first << " ";
		cout << it->second.second << endl;
	}
	cout.flush();
}

void
test_get(kvs_client *kc) {
	cout << "TIMEDOUT GET" << endl;
	kvs_protocol::status r_status;
	string r_value;
	r_status = kc->get(1, r_value);
	if (r_status != kvs_protocol::TIMEOUT) {
		cout << "GET returned a wrong status: " << r_status << endl;
		exit(1);
	}
	cout << "TIMEDOUT GET OK" << endl;
	//wait for the connection to be re-established.
	sleep(2);
}

void
test_put(kvs_client *kc) {
	cout << "TIMEDOUT PUT" << endl;
	kvs_protocol::status r_status;
	r_status = kc->put(1, "abc");
	if (r_status != kvs_protocol::TIMEOUT) {
		cout << "PUT returned a wrong status: " << r_status << endl;
		exit(1);
	}
	cout << "TIMEDOUT PUT OK" << endl;
	//wait for the connection to be re-established.
	sleep(2);
}

void
good_put(kvs_client *kc) {
	cout << "GOOD PUT" << endl;
	kvs_protocol::status r_status;
	r_status = kc->put(1, "abc");
	if (r_status != kvs_protocol::OK) {
		cout << "PUT returned a wrong status: " << r_status << endl;
		exit(1);
	}
	cout << "GOOD PUT OK" << endl;
}

int
main() {
	map<server_name, server_address> table;
	cons_table(table);
	kvs_client *kc = new kvs_client(table);
	//wait for connections to be established.
	sleep(3);

	//nothing
	test_get(kc);
	test_put(kc);
	//incomplete length part
	test_get(kc);
	test_put(kc);
	//incomplete status part
	test_get(kc);
	test_put(kc);
	//incomplete value part
	test_get(kc);
	//good put
	good_put(kc);

	return 0;
}




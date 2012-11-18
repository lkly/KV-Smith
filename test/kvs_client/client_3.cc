#include "kvs_client.h"
#include <map>
#include <string>
#include <sstream>
#include <iostream>

using namespace std;

//this should reflect the size of the array servers correctly.
int servers_size = 3;

const char *servers[] = {
	"A 127.0.0.1 10000",
	"B 127.0.0.1 10001",
	"C 127.0.0.1 10002",
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
test_get_ok(kvs_client * kc, kvs_protocol::key key, const char *value) {
	cout << "GET_OK " << key << endl;
	kvs_protocol::status r_status;
	string r_value;
	r_status = kc->get(key, r_value);
	if (r_status != kvs_protocol::OK) {
		cout << "GET_OK returned a wrong status: " << r_status << endl;
		exit(1);
	}
	if (r_value != value) {
		cout << "GET_OK returned a wrong value: " << r_value << endl;
		exit(1);
	}
	cout << "GET_OK " << key << " OK" << endl;
}

void
test_get_to(kvs_client * kc, kvs_protocol::key key, const char *value) {
	cout << "GET_TO " << key << endl;
	kvs_protocol::status r_status;
	string r_value;
	r_status = kc->get(key, r_value);
	if (r_status != kvs_protocol::TIMEOUT) {
		cout << "GET_TO returned a wrong status: " << r_status << endl;
		exit(1);
	}
	if (r_value != value) {
		cout << "GET_TO returned a wrong value: " << r_value << endl;
		exit(1);
	}
	cout << "GET_TO " << key << " OK" << endl;
}

void
test_put_ok(kvs_client * kc, kvs_protocol::key key, const char *value) {
	cout << "PUT_OK " << key << endl;
	kvs_protocol::status r_status;
	r_status = kc->put(key, value);
	if (r_status != kvs_protocol::OK) {
		cout << "PUT_OK returned a wrong status: " << r_status << endl;
		exit(1);
	}
	cout << "PUT_OK " << key << " OK" << endl;
}

void
test_put_to(kvs_client * kc, kvs_protocol::key key, const char *value) {
	cout << "PUT_TO " << key << endl;
	kvs_protocol::status r_status;
	r_status = kc->put(key, value);
	if (r_status != kvs_protocol::TIMEOUT) {
		cout << "PUT_TO returned a wrong status: " << r_status << endl;
		exit(1);
	}
	cout << "PUT_TO " << key << " OK" << endl;
}

int
main() {
	map<server_name, server_address> table;
	cons_table(table);
	kvs_client *kc = new kvs_client(table);
	//wait for connections to be established.
	sleep(3);

	//A
	test_get_ok(kc, 1, "1");
	test_get_ok(kc, 2, "2");
	test_put_to(kc, 1, "1");
	//A failed, B
	test_get_ok(kc, 3, "3");
	test_put_ok(kc, 1, "1");
	test_get_to(kc, 4, "");
	//B failed, C
	test_get_ok(kc, 4, "4");
	test_put_ok(kc, 2, "2");
	test_put_ok(kc, 3, "3");
	test_put_to(kc, 4, "4");
	//A restarted, C failed, A
	test_get_ok(kc, 5, "5");
	test_put_to(kc, 4, "4");
	//A failed, C restarted, C
	test_put_ok(kc, 4, "4");
	test_put_ok(kc, 5, "5");

	return 0;
}




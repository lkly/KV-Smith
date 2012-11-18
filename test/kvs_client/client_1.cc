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
test_get(kvs_client * kc, kvs_protocol::key key, const char *value, kvs_protocol::status status) {
	cout << "GET " << key << endl;
	kvs_protocol::status r_status;
	string r_value;
	r_status = kc->get(key, r_value);
	if (r_status != status) {
		cout << "GET returned a wrong status: " << r_status << endl;
		exit(1);
	}
	if (r_value != value) {
		cout << "GET returned a wrong value: " << r_value << endl;
		exit(1);
	}
	cout << "GET " << key << " OK" << endl;
}

void
test_put(kvs_client * kc, kvs_protocol::key key, const char *value, kvs_protocol::status status) {
	cout << "PUT " << key << endl;
	kvs_protocol::status r_status;
	r_status = kc->put(key, value);
	if (r_status != status) {
		cout << "PUT returned a wrong status: " << r_status << endl;
		exit(1);
	}
	cout << "PUT " << key << " OK" << endl;
}

int
main() {
	map<server_name, server_address> table;
	cons_table(table);
	kvs_client *kc = new kvs_client(table);
	//wait for connections to be established.
	sleep(3);

	//ok->ok
	test_get(kc, 1, "abc", kvs_protocol::OK);
	test_put(kc, 1, "abc", kvs_protocol::OK);
	test_get(kc, 2000, "abcdefg", kvs_protocol::OK);
	test_put(kc, 2000, "abcdefg", kvs_protocol::OK);
	//not_primary->retry
	test_get(kc, 1, "", kvs_protocol::RETRY);
	test_put(kc, 1, "abc", kvs_protocol::RETRY);
	test_get(kc, 2000, "", kvs_protocol::RETRY);
	test_put(kc, 2000, "abcdefg", kvs_protocol::RETRY);
	//retry->retry
	test_get(kc, 1, "", kvs_protocol::RETRY);
	test_put(kc, 1, "abc", kvs_protocol::RETRY);
	test_get(kc, 2000, "", kvs_protocol::RETRY);
	test_put(kc, 2000, "abcdefg", kvs_protocol::RETRY);
	//timeout->timeout
	test_get(kc, 1, "", kvs_protocol::TIMEOUT);
	test_put(kc, 1, "abc", kvs_protocol::TIMEOUT);
	test_get(kc, 2000, "", kvs_protocol::TIMEOUT);
	test_put(kc, 2000, "abcdefg", kvs_protocol::TIMEOUT);

	return 0;
}




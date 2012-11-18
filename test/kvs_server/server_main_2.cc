#include "kvs_server.h"
#include <vector>

const char *data[] = {
	"any attending frog glances a meat",
	"the prostitute overlaps opposite the unknown",
	"why cant the record worm refrain",
	"a numeral overtone crowns the custard",
	"the laboratory mutters inside a retrieval",
};

extern int counter[];
extern int counter_100[];
extern int counter_10000[];

vector<vector<string> > v_data;

const char *addr = "127.0.0.1 10000";

void
cons_v_data() {
	int i = 0;
	for (; i < 5; i++) {
		stringstream ss;
		ss << data[i];
		vector<string> v;
		int j = 0;
		for (; j < 6; j++) {
			string str;
			ss >> str;
			v.push_back(str);
		}
		v_data.push_back(v);
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

kvs_server *server;

void*
start_server(void *pt) {
	server->start();
}

void
check_put(int key, string value, cs_protocol::status status) {
	stringstream req_ss;
	req_ss << cs_protocol::PUT;
	req_ss << ' ';
	req_ss << key;
	req_ss << ' ';
	req_ss << value;
	string req = req_ss.str();
	string res;
	server->callback(req, res);
	stringstream res_ss;
	res_ss << status;
	if (res != res_ss.str()) {
		cout << "put(" << status << ") returned a wrong status: " << res << endl;
		exit(1);
	}
}

void
check_get(int key, string value, cs_protocol::status status) {
	stringstream req_ss;
	req_ss << cs_protocol::GET;
	req_ss << ' ';
	req_ss << key;
	string req = req_ss.str();
	string res;
	server->callback(req, res);
	stringstream res_ss;
	res_ss << status;
	if (value.length() != 0) {
		res_ss << ' ';
		res_ss << value;
	}
	if (res != res_ss.str()) {
		cout << "get(" << status << " " << value << ") returned a wrong status: " << res << endl;
		exit(1);
	}
}

void
check_good_put(int key, string value) {
	cout << "CHECK good put " << key << endl;
	check_put(key, value, cs_protocol::OK);
}

void
check_good_get(int key, string value) {
	cout << "CHECK good get " << key << endl;
	check_get(key, value, cs_protocol::OK);
}

//independent
void *
i_put_and_get(void *pt) {
	int id = *(int *)pt;
	vector<string> v = v_data[id];
	check_good_put(id, v[0]);
	check_good_put(id+10, v[1]);
	check_good_get(id+10, v[1]);
	check_good_put(id+20, v[2]);
	check_good_get(id, v[0]);
	check_good_get(id+20, v[2]);
	check_good_put(id+30, v[3]);
	check_good_put(id+40, v[4]);
	check_good_get(id+30, v[3]);
	check_good_put(id+50, v[5]);
	check_good_get(id+50, v[5]);
	check_good_get(id+40, v[4]);
	pthread_exit(NULL);
}

void
check_counter() {
	int i = 0;
	for (; i < 5; i++) {
		if (counter[i] != 6) {
			cout << "COUNTER " << i << " is " << counter[i] << endl;
			exit(1);
		}
	}
}


void
test_1() {
	cout << "TEST 1" << endl;
	pthread_t th[5];
	int id[5] = {0, 1, 2, 3, 4};
	int i = 0;
	for (; i < 5; i++) {
		if (pthread_create(&th[i], NULL, i_put_and_get, (void *)&(id[i])) < 0) {
			cout << "CAN'T start test_1 thread." << endl;
			exit(1);
		}
	}
	i = 0;
	for (; i < 5; i++) {
		pthread_join(th[i], NULL);
	}
	check_counter();
	cout << "TEST 1 OK" << endl;
}

//conflicting
void *
c_get(void *pt) {
	int i = 0;
	for (; i < 3; i++) {
		check_good_get(20, "frog");
	}
	i = 0;
	for (; i < 4; i++) {
		check_good_get(43, "the");
	}
	pthread_exit(NULL);
}

void
test_2() {
	cout << "TEST 2" << endl;
	pthread_t th[5];
	int i = 0;
	for (; i < 5; i++) {
		if (pthread_create(&th[i], NULL, c_get, NULL) < 0) {
			cout << "CAN'T start test_2 thread." << endl;
			exit(1);
		}
	}
	i = 0;
	for (; i < 5; i++) {
		pthread_join(th[i], NULL);
	}
	cout << "TEST 2 OK" << endl;
}

//conflicting
void *
c_put(void *pt) {
	int i = 0;
	int id = *(int *)pt;
	stringstream ss;
	ss << id;
	for (; i < 3; i++) {
		check_good_put(100, ss.str());
	}
	i = 0;
	for (; i < 4; i++) {
		check_good_put(10000, ss.str());
	}
	pthread_exit(NULL);
}

void
check_counters() {
	int i = 0;
	for (; i < 5; i++) {
		if (counter_100[i] != 3) {
			cout << "COUNTER_100 " << i << " is " << counter_100[i] << endl;
			exit(1);
		}
	}
	i = 0;
	for (; i < 5; i++) {
		if (counter_10000[i] != 4) {
			cout << "COUNTER_10000 " << i << " is " << counter_10000[i] << endl;
			exit(1);
		}
	}
}

void
test_3() {
	cout << "TEST 3" << endl;
	int id[5] = {0, 1, 2, 3, 4};
	pthread_t th[5];
	int i = 0;
	for (; i < 5; i++) {
		if (pthread_create(&th[i], NULL, c_put, (void *)&(id[i])) < 0) {
			cout << "CAN'T start test_3 thread." << endl;
			exit(1);
		}
	}
	i = 0;
	for (; i < 5; i++) {
		pthread_join(th[i], NULL);
	}
	check_counters();
	cout << "TEST 3 OK" << endl;
}

//conflicting
void *
c_put_and_get(void *pt) {
	int id = *(int *)pt;
	if (id != 0) {
		//don't do get too early.
		sleep(1);
	}
	if (id == 0) {
		check_good_put(20, "ice");
	}
	check_good_get(20, "ice");
	pthread_exit(NULL);
}

void
test_4() {
	cout << "TEST 4" << endl;
	int id[5] = {0, 1, 2, 3, 4};
	pthread_t th[5];
	int i = 0;
	for (; i < 5; i++) {
		if (pthread_create(&th[i], NULL, c_put_and_get, (void *)&(id[i])) < 0) {
			cout << "CAN'T start test_4 thread." << endl;
			exit(1);
		}
	}
	i = 0;
	for (; i < 5; i++) {
		pthread_join(th[i], NULL);
	}
	cout << "TEST 4 OK" << endl;
}

void
check_np_put(int key, string value) {
	cout << "check np put " << key << endl;
	check_put(key, value, cs_protocol::NOT_PRIMARY);
}

void
check_np_get(int key, string value) {
	cout << "check np get " << key << endl;
	string str;
	check_get(key, str, cs_protocol::NOT_PRIMARY);
}

void *
f_put_and_get(void *pt) {
	int id = *(int *)pt;
	int i = 0;
	for (; i < 10; i++) {
		check_np_put(i+id*10, "dull");
		check_np_get(id*10 + 10 -i, "dull");
	}
	pthread_exit(NULL);
}

void
test_5() {
	//wait server lose mastership
	sleep(2);
	cout << "TEST 5" << endl;
	int id[3] = {0, 1, 2};
	pthread_t th[3];
	int i = 0;
	for (; i < 3; i++) {
		if (pthread_create(&th[i], NULL, f_put_and_get, (void *)&(id[i])) < 0) {
			cout << "CAN'T start test_5 thread." << endl;
			exit(1);
		}
	}
	i = 0;
	for (; i < 3; i++) {
		pthread_join(th[i], NULL);
	}
	cout << "TEST 5 OK" << endl;
}

void check_test_6();

void *
r_get(void *pt) {
	int i = 0;
	for (; i < 10; i++) {
		stringstream ss;
		ss << (1000+i);
		check_good_get(1000+i, ss.str());
	}
	check_good_get(43, "the");
	check_good_get(20, "the");
	pthread_exit(NULL);
}


void
test_6() {
	check_test_6();
	sleep(1);
	cout << "TEST 6" << endl;
	pthread_t th[4];
	int i = 0;
	for (; i < 4; i++) {
		if (pthread_create(&th[i], NULL, r_get, NULL) < 0) {
			cout << "CAN'T start test_6 thread." << endl;
			exit(1);
		}
	}
	i = 0;
	for (; i < 4; i++) {
		pthread_join(th[i], NULL);
	}
	cout << "TEST 6 OK" << endl;
}

void
check_to_put(int key, string value) {
	cout << "check to put " << key << endl;
	check_put(key, value, cs_protocol::TIMEOUT);
}

void
check_rt_put(int key, string value) {
	cout << "check rt put " << key << endl;
	check_put(key, value, cs_protocol::RETRY);
}

void *
f_put(void *pt) {
	int id = *(int *)pt;
	if (id == 0) {
		check_to_put(500, "dull");
	}
	if (id == 1) {
		//let 0 do put first
		sleep(1);
		check_rt_put(600, "dull");
	}
	if (id == 2) {
		//do after 0 returns
		sleep(3);
		check_np_put(20, "dull");
	}
	pthread_exit(NULL);
}

void
test_7() {
	cout << "TEST 7" << endl;
	pthread_t th[3];
	int id[3] = {0, 1, 2};
	int i = 0;
	for (; i < 3; i++) {
		if (pthread_create(&th[i], NULL, f_put, (void *)&(id[i])) < 0) {
			cout << "CAN'T start test_7 thread." << endl;
			exit(1);
		}
	}
	i = 0;
	for (; i < 3; i++) {
		pthread_join(th[i], NULL);
	}
	cout << "TEST 7 OK" << endl;
}

int
main() {
	map<server_name, server_address> table;
	server_address address;
	cons_addr(address);
	cons_v_data();
	string name;
	name = "A";
	server = new kvs_server(name, address, table);
	pthread_t s_th;
	if (pthread_create(&s_th, NULL, start_server, NULL) < 0) {
		cout << "CAN'T start server." << endl;
		exit(1);
	}
	//wait server to be master.
	sleep(2);
	test_1();
	test_2();
	test_3();
	test_4();
	test_5();
	test_6();
	test_7();
	return 0;
}




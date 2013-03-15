#include "kvs_client.h"
#include <map>
#include <string>
#include <sstream>
#include <iostream>
#include <time.h>
#include <sys/time.h>
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>

using namespace std;

unsigned int duration;
int status;

kvs_client *kc;
string r_value;
string w_value;
unsigned int rwkey;

//a thread-based timer
void *
timer(void *arg) {
	int times;
	times = *((int *)arg);
	while (times != 0) {
		sleep(duration);
		pthread_mutex_lock(&status_lock);
		status = 1;
		pthread_mutex_unlock(&status_lock);
		times--;
	}
}

void
cons_table(map<server_name, server_address> &table) {
	fstream fs("kvs-c.config", ios::in);
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

kvs_protocol::status
send_request(int ratio) {
	kvs_protocol::status r;
	string r_value;
	if (rand%10 < ratio) {
		string value;
		value = ""
	} else {
		r = 	
	r_status = kc->get(key, r_value);
	}
	return r;
}

void
eval_1() {
	pthread_t mytimer;
	int times;
	times = 1;
	pthread_create((&mytimer, NULL, timer, (void *)&times);
	int progress;
	progress = 0;
	map<int, int> counts;
	while (progress != 11) {
		int mystatus;
		int count;
		count = 0;
		mystatus = 0;
		while (1) {
			pthread_mutex_lock(&status_lock);
			mystatus = status;
			status = 0;
			pthread_mutex_unlock(&status_lock);
			if (mystatus == 0) {
				int r;
				r = send_request(3);
				assert(r == kvs_protocol::OK);
				count++;
			} else {
				break;
			}
		}
		counts[progress] = count;
		progress++;
	}
	progress = 0;
	cout << "eval_3:";
	while (progress != 11) {
		cout << " " << counts[progress];
		progress++;
	}
	pthread_join(mytimer, NULL);
}

void
eval_2() {
	struct timeval start;
	struct timeval end;
	while (1) {
		int r;
		r = send_request(3);
		if (r != kvs_protocol::OK) {
			gettimeofday(&start, NULL);
			break;
		}
	}
	while (1) {
		int r;
		r = send_request(3);
		if (r == kvs_protocol::OK) {
			gettimeofday(&end, NULL);
			break;
		}
	}
	unsigned int interval;
	interval = 0;
	interval += (end.tv_usec - start.tv_usec)/1000;
	interval += (end.tv_sec - start.tv_sec)*1000;
	cout << "eval_2: " << interval << endl;
}

void
eval_3() {
	pthread_t mytimer;
	int times;
	times = 10;
	pthread_create((&mytimer, NULL, timer, (void *)&times);
	int progress;
	progress = 0;
	map<int, int> counts;
	while (progress != 11) {
		int mystatus;
		int count;
		count = 0;
		mystatus = 0;
		while (1) {
			pthread_mutex_lock(&status_lock);
			mystatus = status;
			status = 0;
			pthread_mutex_unlock(&status_lock);
			if (mystatus == 0) {
				int r;
				r = send_request(progress);
				assert(r == kvs_protocol::OK);
				count++;
			} else {
				break;
			}
		}
		counts[progress] = count;
		progress++;
	}
	progress = 0;
	cout << "eval_3:";
	while (progress != 11) {
		cout << " " << counts[progress];
		progress++;
	}
	pthread_join(mytimer, NULL);
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
main(int argc, char *argv[]) {
	//client_name, eval_number, eval-specific:timer_duration, sample_times
	if (argc != 3) {
		cout << "wrong number of arguments" << endl;
		exit(1);
	}
	map<server_name, server_address> table;
	cons_table(table);
	kc = new kvs_client(table);
	//wait for connections to be established.
	sleep(3);

	stringstream ss;
	ss << argv[1];
	int eval_number;
	ss >> eval_number;
	ss << argv[2];
	ss >> duration;
	srand(getpid());

	switch (eval_num) {
		case 1:
			eval_1();
			break;
		case 2:
			eval_2();
			break;
		case 3:
			eval_3();
			break;
		default:
			cout << "unknown eval number: " << eval_number << endl;
			exit(1);
	}
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


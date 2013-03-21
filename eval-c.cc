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
unsigned int rw_ratio;
pthread_mutex_t status_lock;
int status;

kvs_client *kc;
int myid;

string thevalue = "@1";

string values[] = {"@2", "@3", "@4", "@5"};


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
	pthread_exit(NULL);
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
send_request(int ratio, kvs_protocol::key key, string &rvalue, string &wvalue, int &rw) {
	kvs_protocol::status r;
	if (rand%10 < ratio) {
		r = kc->put(key, wvalue);
		rw = 1;
	} else {
		r = kc->get(key, rvalue);
		rw = 0;
	}
	return r;
}

void
eval_1() {
	//prepare
	kvs_protocol::key key;
	key = myid;
	string rvalue;

	string wvalue;


	//start timer
	pthread_t mytimer;
	int times;
	times = 1;
	pthread_create((&mytimer, NULL, timer, (void *)&times);

	int mystatus;
	int count;
	count = 0;
	mystatus = 0;
	int index = 0;
	while (1) {
		wvalue = values[index];
		pthread_mutex_lock(&status_lock);
		mystatus = status;
		pthread_mutex_unlock(&status_lock);
		if (mystatus == 0) {
			int r;
			int rw;
			r = send_request(rw_ratio, key, rvalue, wvalue, rw);
			assert(r == kvs_protocol::OK);
			if (rw == 0) {
				assert(rvalue == thevalue);
			} else {
				thevalue = wvalue;
			}
			count++;
		} else {
			break;
		}
		index = (index++)%4
	}
	cout << " " << count;
	pthread_join(mytimer, NULL);
}

void
eval_2() {
	//prepare
	kvs_protocol::key key;
	key = myid;
	string rvalue;
	int index = 0;
	string wvalue;

	struct timeval start;
	struct timeval end;
	while (1) {
		wvalue = values[index];
		int r;
		int rw;
		r = send_request(3, key, rvalue, wvalue, rw);
		if (r != kvs_protocol::OK) {
			gettimeofday(&start, NULL);
			break;
		} else {
			if (rw == 0) {
				assert(rvalue == thevalue);
			} else {
				thevalue = wvalue;
			}
		}
		index = (index++)%4;
	}
	while (1) {
		int r;
		int rw;
		r = send_request(3, key, rvalue, wvalue, rw);
		if (r == kvs_protocol::OK) {
			gettimeofday(&end, NULL);
			break;
		}
	}
	unsigned int interval;
	interval = 0;
	interval += (end.tv_usec - start.tv_usec)/1000;
	interval += (end.tv_sec - start.tv_sec)*1000;
	cout << " " << interval << endl;
}
/*
void
eval_3(int points) {
	//prepare
	kvs_protocol::key key;
	key = myid;
	string rvalue;
	stringstream ss;
	ss << "@w:";
	ss << myid;
	string wvalue;
	ss >> wvalue;
	int progress;
	progress = 0;
	map<int, int> counts;

	//start timer
	pthread_t mytimer;
	int times;
	times = points;
	pthread_create((&mytimer, NULL, timer, (void *)&times);

	while (progress != points) {
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
				r = send_request(progress, key, rvalue, wvalue);
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
	while (progress != points) {
		cout << " " << counts[progress];
		progress++;
	}
	pthread_join(mytimer, NULL);
}
*/
int
main(int argc, char *argv[]) {
	//client_id, eval_number, durtion, rw_ratio
	//setup
	if (argc != 5) {
		cout << "wrong number of arguments for eval" << endl;
		exit(1);
	}
	map<server_name, server_address> table;
	cons_table(table);
	kc = new kvs_client(table);
	//wait for connections to be established.
	sleep(3);

	stringstream ss;
	ss << argv[1];
	ss >> myid;
	pthread_mutex_init(&status_lock, NULL);
	status = 0;
	srand(getpid());

	//start
	int eval_number;
	ss << argv[2];
	ss >> eval_number;


	ss << argv[3];
	ss >> duration;


	ss << argv[4];
	ss >> rw_ratio;

	switch (eval_number) {
		case 1:
			eval_1();
			break;
		case 2:
			eval_2();
			break;
		case 3:
			eval_1();
			break;
		default:
			cout << "unknown eval number: " << eval_number << endl;
			exit(1);
	}
	delete kc;
	return 0;
}


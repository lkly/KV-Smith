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
#include <pthread.h>
#include <fstream>
#include <stdio.h>

//for eval or just test purpose
#define EVAL

#define EVAL_PIPE 15

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
	while (!fs.eof()) {
		stringstream ss;
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
	if (rand()%10 < ratio) {
		r = kc->put(key, wvalue);
		rw = 1;
	} else {
		r = kc->get(key, rvalue);
		rw = 0;
	}
	return r;
}

void
eval_performance() {
	//prepare
	kvs_protocol::key key;
	key = myid;
	string rvalue;
	string wvalue;

	//start timer
	pthread_t mytimer;
	int times;
	times = 1;
	pthread_create(&mytimer, NULL, timer, (void *)&times);

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
			if (r != kvs_protocol::OK) {
				cout << r << endl;
			}
			assert(r == kvs_protocol::OK);
			if (rw == 0) {
				assert(rvalue == thevalue);
			} else {
				thevalue = wvalue;
				index++;
				index = index%4;
			}
			count++;
		} else {
			break;
		}
	}
	cout << '#' << count << '#' << endl;
#ifdef EVAL
	char buff[30];
	int len;
	len = snprintf(buff, sizeof(buff), "%d", count);
//	buff[len++] = '#';
//	buff[len] = 0;
	assert(write(EVAL_PIPE, buff, len) == len);
#endif
	pthread_join(mytimer, NULL);
}

void
eval_failover() {
	//prepare
	kvs_protocol::key key;
	key = myid;
	string rvalue;
	int index;
	index = 0;
	string wvalue;

	struct timeval start;
	struct timeval end;
	while (1) {
		wvalue = values[index];
		int r;
		int rw;
		r = send_request(3, key, rvalue, wvalue, rw);
		cout << "send request and get the return value: " << r << endl;
		if (r != kvs_protocol::OK) {
			gettimeofday(&start, NULL);
			break;
		} else {
			if (rw == 0) {
				assert(rvalue == thevalue);
			} else {
				thevalue = wvalue;
				index++;
				index = index%4;
			}
		}
	}
	cout << "detect master change" << endl;
	while (1) {
		int r;
		int rw;
		r = send_request(3, key, rvalue, wvalue, rw);
		cout << "send request and get the return value: " << r << endl;
		if (r == kvs_protocol::OK) {
			gettimeofday(&end, NULL);
			break;
		}
		sleep(1);
	}
	unsigned int interval;
	interval = 0;
	interval += (end.tv_usec - start.tv_usec)/1000;
	interval += (end.tv_sec - start.tv_sec)*1000;
	cout << '#' << interval << '#' << endl;
#ifdef EVAL
	char buff[30];
	int len;
	len = snprintf(buff, sizeof(buff), "%d", interval);
//	buff[len++] = '#';
//	buff[len]=0;
	assert(write(EVAL_PIPE, buff, len) == len);
#endif
}

void
warmup() {
	int r;
	int key;
	key = myid;
	string rvalue;
	while (1) {
		r = kc->get(key, rvalue);
		if (r == kvs_protocol::OK) {
			assert(rvalue == thevalue);
			return;
		}
		sleep(1);
	}
}

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
	print_table(table);
	kc = new kvs_client(table);
	//wait for connections to be established.
	sleep(3);

	pthread_mutex_init(&status_lock, NULL);
	status = 0;
	srand(getpid());

	//start
	myid = atoi(argv[1]);

	int eval_number;
	eval_number = atoi(argv[2]);

	duration = atoi(argv[3]);

	rw_ratio = atoi(argv[4]);

	warmup();

	switch (eval_number) {
		case 1:
			eval_performance();
			break;
		case 2:
			eval_failover();
			break;
		case 3:
			eval_performance();
			break;
		default:
			cout << "unknown eval number: " << eval_number << endl;
			exit(1);
	}
	delete kc;
#ifdef EVAL
	close(EVAL_PIPE);
#endif
	return 0;
}


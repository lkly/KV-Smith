#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <string>
#include <utility>
#include "utils.h"
#include <sstream>
#include "common.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <map>
#include <vector>
#include <iostream>

using namespace std;

const char *servers[] = {
	"A 127.0.0.1 20000",
	"B 127.0.0.1 20001",
	"C 127.0.0.1 20002",
};

map<server_name, server_address> table;
map<string, string> col_B;
map<string, string> col_C;

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

void *
test(void *pt) {
	int id = *(int *)pt;
	string name;
	if (id == 0) {
		name = "B";
	}
	if (id == 1) {
		name = "C";
	}
	string ip_addr;
	string port;
	ip_addr = table[name].first;
	port = table[name].second;
	int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0) {
		cout << "CAN'T open socket." << endl;
		exit(1);
	}
	struct sockaddr_in server_addr;
	bzero(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(ip_addr.c_str());
	server_addr.sin_port = htons(atoi(port.c_str()));
	if (bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
		cout << "CAN'T bind socket." << endl;
		exit(1);
	}
	bzero(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(table["A"].first.c_str());
	server_addr.sin_port = htons(atoi(table["A"].second.c_str()));
	int i = 0;
	for (; i < 20; i++) {
		stringstream ss;
		ss << name;
		ss << ' ';
		ss << i;
		sendto(sockfd, ss.str().c_str(), ss.str().length(), 0, (struct sockaddr*)(&server_addr), sizeof(struct sockaddr_in));
		cout << name << "->A: " << i << endl;
		usleep(100000);
	}
	i = 0;
	int r;
	char buff[10];
	for (; i < 20; i++) {
		r = recvfrom(sockfd, buff, 9, 0, NULL, NULL);
		buff[r] = 0;
		stringstream r_ss;
		r_ss << buff;
		if (name == "B") {
			col_B[r_ss.str()] = r_ss.str();
		}
		if (name == "C") {
			col_C[r_ss.str()] = r_ss.str();
		}
	}
	sleep(1);
	pthread_exit(NULL);
}

int
main() {
	cons_table(table);
	pthread_t th_0;
	int id_0 = 0;
	pthread_t th_1;
	int id_1 = 1;
	pthread_create(&th_0, NULL, test, (void *)&id_0);
	pthread_create(&th_1, NULL, test, (void *)&id_1);
	pthread_join(th_0, NULL);
	pthread_join(th_1, NULL);
	int i = 0;
	for (; i < 20; i++) {
		stringstream ss;
		ss << "A";
		ss << ' ';
		ss << i;
		if (col_B[ss.str()] != ss.str()) {
			cout << "WRONG message(" << ss.str() << "): " << col_B[ss.str()] << endl;
			exit(1);
		}
	}
	i = 0;
	for (; i < 20; i++) {
		stringstream ss;
		ss << "A";
		ss << ' ';
		ss << i;
		if (col_C[ss.str()] != ss.str()) {
			cout << "WRONG message(" << ss.str() << "): " << col_C[ss.str()] << endl;
			exit(1);
		}
	}
	cout << "TEST OK" << endl;
	return 0;
}




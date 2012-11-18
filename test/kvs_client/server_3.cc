#include <iostream>
#include "client-server_protocol.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <vector>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include <string>

using namespace std;

//this should reflect the size of the array servers correctly.
int servers_size = 3;

const char *servers[] = {
	"127.0.0.1 10000",
	"127.0.0.1 10001",
	"127.0.0.1 10002",
};

vector<int> sockfds;

void
start_servers(vector<int> &sockfds) {
	int i = 0;
	for (; i < servers_size; i++) {
		stringstream ss;
		ss << servers[i];
		string ip_addr;
		string port;
		ss >> ip_addr;
		ss >> port;
		int sockfd;
		sockfd = socket(AF_INET, SOCK_STREAM, 0);
		if (sockfd < 0) {
			cout << "SERVER failed to open socket." << endl;
			exit(1);
		}
		struct sockaddr_in addr;
		bzero(&addr, sizeof(addr));
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = inet_addr(ip_addr.c_str());
		addr.sin_port = htons(atoi(port.c_str()));
		if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
			cout << "SERVER failed to bind socket." << endl;
			exit(1);
		}
		listen(sockfd, -1);
		sockfds.push_back(sockfd);
	}
}

void
test_get(int sockfd, int key, const char *value) {
	cout << "TEST GET " << key << endl;
	stringstream ss;
	ss << cs_protocol::GET;
	ss << ' ';
	ss << key;
	//20 bytes is enough.
	char buffer[20];
	int next = 0;
	while (1) {
		int r = recv(sockfd, buffer+next, 19-next, 0);
		next += r;
		if (next >= (ss.str().length()+4)) {
			break;
		}
	}
	buffer[next] = 0;
	stringstream r_ss;
	r_ss << buffer+4;
	int len = ntohl(*(int *)buffer);
	if (r_ss.str() != ss.str() || len != ss.str().length()) {
		cout << "MALFORMED get request: " << r_ss.str() << endl;
		cout << "length: " << len << endl;
		exit(1);
	}
	stringstream result;
	result << cs_protocol::OK;
	result << ' ';
	result << value;
	len = htonl(result.str().length());
	send(sockfd, &len, 4, 0);
	send(sockfd, result.str().c_str(), ntohl(len), 0);
	cout << "TEST GET " << key << " OK" << endl;
}

void
test_put(int sockfd, int key, const char *value) {
	cout << "TEST PUT " << key << endl;
	stringstream ss;
	ss << cs_protocol::PUT;
	ss << ' ';
	ss << key;
	ss << ' ';
	ss << value;
	//30 bytes is enough.
	char buffer[30];
	int next = 0;
	while (1) {
		int r = recv(sockfd, buffer+next, 29-next, 0);
		next += r;
		if (next >= (ss.str().length()+4)) {
			break;
		}
	}
	buffer[next] = 0;
	stringstream r_ss;
	r_ss << buffer+4;
	int len = ntohl(*(int *)buffer);
	if (r_ss.str() != ss.str() || len != ss.str().length()) {
		cout << "MALFORMED put request: " << r_ss.str() << endl;
		cout << "length: " << len << endl;
		exit(1);
	}
	stringstream result;
	result << cs_protocol::OK;
	len = htonl(result.str().length());
	send(sockfd, &len, 4, 0);
	send(sockfd, result.str().c_str(), ntohl(len), 0);
	cout << "TEST PUT " << key << " OK" << endl;
}

void *
A(void *dull) {
	int sockfd = accept(sockfds[0], NULL, NULL);
	test_get(sockfd, 1, "1");
	test_get(sockfd, 2, "2");
	//A failed
	sockfd = accept(sockfds[0], NULL, NULL);
	//A restarted
	test_get(sockfd, 5, "5");
	pthread_exit(NULL);
}

void *
B(void *dull) {
	int sockfd = accept(sockfds[1], NULL, NULL);
	test_get(sockfd, 3, "3");
	test_put(sockfd, 1, "1");
	//B failed
	//may raise SIGPIPE
	close(sockfds[1]);
	pthread_exit(NULL);
}

void *
C(void *dull) {
	int sockfd = accept(sockfds[2], NULL, NULL);
	test_get(sockfd, 4, "4");
	test_put(sockfd, 2, "2");
	test_put(sockfd, 3, "3");
	//C failed
	sockfd = accept(sockfds[2], NULL, NULL);
	//C restarted
	test_put(sockfd, 4, "4");
	test_put(sockfd, 5, "5");
	pthread_exit(NULL);
}

int
main() {
	start_servers(sockfds);
	int rc;
	pthread_t th_a;
	rc = pthread_create(&th_a, NULL, A, NULL);
	if (rc < 0) {
		cout << "FAIL to start A." << endl;
		exit(1);
	}
	pthread_t th_b;
	rc = pthread_create(&th_b, NULL, B, NULL);
	if (rc < 0) {
		cout << "FAIL to start B." << endl;
		exit(1);
	}
	pthread_t th_c;
	rc = pthread_create(&th_c, NULL, C, NULL);
	if (rc < 0) {
		cout << "FAIL to start C." << endl;
		exit(1);
	}
	pthread_join(th_a, NULL);
	pthread_join(th_b, NULL);
	pthread_join(th_c, NULL);
	return 0;
}




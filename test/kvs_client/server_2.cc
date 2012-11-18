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
int servers_size = 1;

const char *servers[] = {
	"127.0.0.1 10000",
};

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
		listen(sockfd, 5);
		sockfds.push_back(sockfd);
	}
}

void
test_get(int sockfd, const char *r, int length) {
	cout << "TEST_GET" << endl;
	int peer_sockfd;
	peer_sockfd = accept(sockfd, NULL, NULL);
	if(peer_sockfd < 0) {
		cout << "SERVER accept returned a bad value: " << peer_sockfd << endl;
		exit(1);
	}
	send(peer_sockfd, r, length, 0);
	cout << "TEST_GET OK" << endl;
}

void
test_put(int sockfd, const char *r, int length) {
	cout << "TEST_PUT" << endl;
	int peer_sockfd;
	peer_sockfd = accept(sockfd, NULL, NULL);
	if(peer_sockfd < 0) {
		cout << "SERVER accept returned a bad value: " << peer_sockfd << endl;
		exit(1);
	}
	send(peer_sockfd, r, length, 0);
	cout << "TEST_PUT OK" << endl;
}

int
main() {
	vector<int> sockfds;
	start_servers(sockfds);
	const char *r_get = "\0\0\0\5\61 abc";
	const char *r_put = "\0\0\0\1\61";

	//nothing
	test_get(sockfds[0], r_get, 0);
	test_put(sockfds[0], r_put, 0);
	//incomplete length part
	test_get(sockfds[0], r_get, 2);
	test_put(sockfds[0], r_put, 2);
	//incomplete status part
	test_get(sockfds[0], r_get, 4);
	test_put(sockfds[0], r_put, 4);
	//incomplete value part
	test_get(sockfds[0], r_get, 7);
	//good put
	test_put(sockfds[0], r_put, 5);

	//don't want SIGPIPE
	//let the peer terminate first
	sleep(3);
	return 0;
}




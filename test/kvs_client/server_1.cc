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
check_get(int sockfd, int key, const char *value, cs_protocol::status status) {
	cout << "CHECK GET " << key << endl;
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
	result << status;
	result << ' ';
	result << value;
	len = htonl(result.str().length());
	send(sockfd, &len, 4, 0);
	send(sockfd, result.str().c_str(), ntohl(len), 0);
	cout << "CHECK GET " << key << " OK" << endl;
}

void
check_put(int sockfd, int key, const char *value, cs_protocol::status status) {
	cout << "CHECK PUT " << key << endl;
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
	result << status;
	len = htonl(result.str().length());
	send(sockfd, &len, 4, 0);
	send(sockfd, result.str().c_str(), ntohl(len), 0);
	cout << "CHECK PUT " << key << " OK" << endl;
}

int
main() {
	vector<int> sockfds;
	start_servers(sockfds);
	int peer_sockfd;
	peer_sockfd = accept(sockfds[0], NULL, NULL);
	if(peer_sockfd < 0) {
		cout << "SERVER accept returned a bad value: " << peer_sockfd << endl;
		exit(1);
	}

	//ok
	check_get(peer_sockfd, 1, "abc", cs_protocol::OK);
	check_put(peer_sockfd, 1, "abc", cs_protocol::OK);
	check_get(peer_sockfd, 2000, "abcdefg", cs_protocol::OK);
	check_put(peer_sockfd, 2000, "abcdefg", cs_protocol::OK);
	//not_primary
	check_get(peer_sockfd, 1, "", cs_protocol::NOT_PRIMARY);
	check_put(peer_sockfd, 1, "abc", cs_protocol::NOT_PRIMARY);
	check_get(peer_sockfd, 2000, "", cs_protocol::NOT_PRIMARY);
	check_put(peer_sockfd, 2000, "abcdefg", cs_protocol::NOT_PRIMARY);
	//retry
	check_get(peer_sockfd, 1, "", cs_protocol::RETRY);
	check_put(peer_sockfd, 1, "abc", cs_protocol::RETRY);
	check_get(peer_sockfd, 2000, "", cs_protocol::RETRY);
	check_put(peer_sockfd, 2000, "abcdefg", cs_protocol::RETRY);
	//timeout
	check_get(peer_sockfd, 1, "", cs_protocol::TIMEOUT);
	check_put(peer_sockfd, 1, "abc", cs_protocol::TIMEOUT);
	check_get(peer_sockfd, 2000, "", cs_protocol::TIMEOUT);
	check_put(peer_sockfd, 2000, "abcdefg", cs_protocol::TIMEOUT);
	
	close(peer_sockfd);
	close(sockfds[0]);
	return 0;
}




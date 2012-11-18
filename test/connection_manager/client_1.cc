#include <iostream>
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

const char *server_addr = "127.0.0.1 10000";

int data_size = 9;

const char *data[] = {
	"abc", "abcdef", "abcdefghi",
	"123", "123456", "123456789",
	"1a2b3d", "xyz123", "000000",
};

void
send_data(int sockfd, int id, int i) {
	stringstream ss;
	ss << id;
	ss << " ";
	ss << data[i];
	int length = ss.str().length();
	length = htonl(length);
	send(sockfd, &length, 4, 0);
	send(sockfd, ss.str().c_str(), ntohl(length), 0);
}


void *
check_data(void *pt) {
	stringstream ss;
	ss << server_addr;
	string ip_addr;
	ss >> ip_addr;
	string port;
	ss >> port;
	int sockfd;
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		cout << "CAN'T open socket." << endl;
		exit(1);
	}
	struct sockaddr_in addr;
	bzero(&addr, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(ip_addr.c_str());
	addr.sin_port = htons(atoi(port.c_str()));
	if (connect(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		cout << "CAN'T connect to server." << endl;
		exit(1);
	}
	int i = 0;
	for (; i < 9; i++) {
		send_data(sockfd, *(int *)pt, i);
		stringstream ss;
		ss << data[i];
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
		if (ss.str() != r_ss.str()) {
			cout << ss.str() << " is not same as " << r_ss.str() << endl;
			exit(1);
		}
		cout << *(int *)pt << " checked data " << i << " ok" << endl;
		//make print fancier.
		usleep(100000 * (*(int *)pt));
	}
	pthread_exit(NULL);
}

int
main() {
	int rc;
	pthread_t th[5];
	int id[5] = {0, 1, 2, 3, 4,};
	int i = 0;
	for (; i < 5; i++) {
		rc = pthread_create(&th[i], NULL, check_data, (void *)&id[i]);
		if (rc < 0) {
			cout << "FAIL to start " << id[i] << endl;
			exit(1);
		}
	}
	i = 0;
	for (; i < 5; i++) {
		pthread_join(th[i], NULL);
	}
	cout << "TEST OK" << endl;
	return 0;
}




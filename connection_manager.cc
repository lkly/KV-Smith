#include "connection_manager.h"
#include "kvs_server.h"

connection_manager::connection_manager(kvs_server *employer) {
	this->employer = employer;
}

connection_manager::~connection_manager() {
	kvs_error("@~connection_manager: connection_manager destructor shouldn't be called!\n");
}

void *
connection_manager::getready(void *obj) {
	connection_manager *cm = new connection_manager((kvs_server *)obj);
	cm->start();
	return NULL;
}

void
connection_manager::start() {
	server_address emp_addr;
	employer->get_address(emp_addr);
	assert(emp_addr.first != "");
	assert(emp_addr.second != "");
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		kvs_error("@start: connection_manager can't open socket!\n");
	}
	struct sockaddr_in addr;
	bzero(&addr, sizeof(addr));
	addr.sin_family = AF_INET;
	//may use emp_addr.first
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(atoi(emp_addr.second.c_str()));

	int yes;
	yes = 1;
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
		kvs_error("@start: connection_manager can't set socket reuseaddr!\n");
	}

	if(bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
		kvs_error("@start: connection_manager can't bind port!\n");
	}
	listen(sockfd, 5);
	int client_sockfd;
	while (1) {
		client_sockfd = accept(sockfd, NULL, NULL);
		if(client_sockfd < 0) {
			kvs_error("@start: connection_manager accept exception!\n");
		}
		pthread_t th;
		context *ctx = new context(employer, client_sockfd);
		if (pthread_create(&th, NULL, connection_manager::worker, (void *)ctx) < 0) {
			kvs_error("@start: connection_manager starting worker fails!\n");
		}
	}
}

void *
connection_manager::worker(void *obj) {
	kvs_server *employer = ((context *)obj)->employer;
	int myfd = ((context *)obj)->worker_fd;
	delete (context *)obj;
	while (1) {
		string request;
		if (!get_request(myfd, request)) {
			break;
		}
		string result;
		employer->callback(request, result);
		if (!return_result(myfd, result)) {
			break;
		}
	}
	close(myfd);
	return NULL;
}

bool
connection_manager::get_request(int fd, string &request) {
	char buffer[501];
	int next = 0;
	stringstream result;
	int length;
	int r;
	while (1) {
		r = recv(fd, buffer+next, 500-next, 0);
		if (r <= 0) {
			return false;
		}
		next += r;
		if (next > 3) {
			break;
		}
	}
	length = ntohl(*((int *)((void *)buffer)));
	buffer[next] = 0;
	result << buffer+4;
	while (1) {
		if (next == length+4) {
			break;
		}
		r = recv(fd, buffer, 500, 0);
		if (r <= 0) {
			return false;
		}
		next += r;
		buffer[r] = 0;
		result << buffer;
	}
	request = result.str();
	return true;
}

bool
connection_manager::return_result(int fd, string &result) {
	int length = result.length();
	length = htonl(length);
	int r = send(fd, &length, 4, NULL);
	assert(r == 4);
	r = send(fd, result.c_str(), result.length(), 0);
	//os buffer should hold first send message, if then fills up,
	//should be signaled by the blocking recv call first.
	if (r >= 0) {
		assert(r == (int)result.length());
	}
	if (r < 0) {
		return false;
	}
	return true;
}



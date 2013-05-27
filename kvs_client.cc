#include "kvs_client.h"
#include <iostream>

kvs_client::kvs_client(map<server_name, server_address> &servers) {
	this->servers = servers;
	server_number = servers.size();
	pthread_mutex_init(&sc_mutex, NULL);
	map<server_name, server_address>::iterator it;
	for (it = servers.begin(); it != servers.end(); it++) {
		server_connections.push_back(make_pair((*it).first, -1));
	}
	signal(SIGPIPE, SIG_IGN);
	primary_server = 0;
	exiting = false;
	if (pthread_create(&cm, NULL, kvs_client::connection_maintainer, (void *)this) < 0) {
		kvs_error("@kvs_client: kvs_client connection maintainer creation fails!\n");
	}
}

kvs_client::~kvs_client() {
	//kvs_error("@~kvs_client: kvs_client destructor shouldn't be called!\n");
	pthread_mutex_lock(&sc_mutex);
	exiting = true;
	pthread_mutex_unlock(&sc_mutex);
	pthread_join(cm, NULL);
	int i;
	for (i = 0; i < server_number; i++) {
		if (server_connections[i].second != -1) {
			close(server_connections[i].second);
		}
	}
}

void *
kvs_client::connection_maintainer(void *obj) {
	((kvs_client *)obj)->connection_maintain();
	pthread_exit(NULL);
	return NULL;
}

void
kvs_client::connection_maintain() {
	while (1) {
		map<server_name, int> dead_servers;
		int i;
		pthread_mutex_lock(&sc_mutex);
		if (exiting) {
			pthread_mutex_unlock(&sc_mutex);
			return;
		}
		for (i = 0; i < server_number; i++) {
			if (server_connections[i].second == -1) {
				dead_servers[server_connections[i].first] = i;
			}
		}
		pthread_mutex_unlock(&sc_mutex);
		try_to_connect(dead_servers);
		//check dead links every 5s
		sleep(3);
	}
}

void
kvs_client::try_to_connect(map<server_name, int> &dead_servers) {
	map<server_name, int>::iterator it;
	for (it = dead_servers.begin(); it != dead_servers.end(); it++) {
		std::cout << "try to connect to " << it->first << std::endl;
		int sockfd;
		if (connecting_done(sockfd, servers[(*it).first])) {
			std::cout << "connecting to " << it->first << " ok" << std::endl;
			pthread_mutex_lock(&sc_mutex);
			assert(server_connections[(*it).second].first == (*it).first);
			assert(server_connections[(*it).second].second == -1);
			server_connections[(*it).second].second = sockfd;
			pthread_mutex_unlock(&sc_mutex);
		}
	}
}

bool
kvs_client::connecting_done(int &sockfd, server_address &address) {
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		kvs_error("@connecting_done: kvs_client open socket fails!\n");
	}
	struct timeval timeout;
	timeout.tv_sec = 5;
	timeout.tv_usec = 0;
	if (setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) < 0) {
		kvs_error("@connecting_done: kvs_client set socketopt SO_SNDTIMEO fails!\n");
	}
	if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
		kvs_error("@connecting_done: kvs_client set socketopt SO_RCVTIMEO fails!\n");
    }
	struct sockaddr_in target_addr;
	bzero(&target_addr, sizeof(target_addr));
	target_addr.sin_family = AF_INET;
	target_addr.sin_addr.s_addr = inet_addr(address.first.c_str());
	target_addr.sin_port = htons(atoi(address.second.c_str()));
	if (connect(sockfd, (struct sockaddr *)&target_addr, sizeof(target_addr)) < 0) {
		perror("connect()");
		close(sockfd);
		return false;
	} else {
		return true;
	}
}

kvs_protocol::status
kvs_client::get(kvs_protocol::key key, string &value) {
	int connection = next_connection();
	if (connection == -1) {
		return kvs_protocol::RETRY;
	}
	stringstream message;
	message << cs_protocol::GET;
	message << ' ';
	message << key;
	int length = message.str().length();
	length = htonl(length);
	int r = send(connection, &length, 4, 0);
	assert(r == 4);
	r = send(connection, message.str().c_str(), ntohl(length), 0);
	//the os buffer should hold the message at lease once, so send's blocking and successive 
	//timeout will be signaled by recv's timeout.
	//enable this assertion only in distributed environment
	//assert(r == (int)ntohl(length));
	string r_message;
	if (get_response(connection, r_message)) {
		string r_value;
		switch (get_r_value(r_message, r_value)) {
			case cs_protocol::NOT_PRIMARY:
			case cs_protocol::RETRY:
				primary_server = next_server(primary_server);
				return kvs_protocol::RETRY;
			case cs_protocol::TIMEOUT:
				primary_server = next_server(primary_server);
				return kvs_protocol::TIMEOUT;
			case cs_protocol::OK:
				value = r_value;
				return kvs_protocol::OK;
		}
	} else {
		update_connection(connection);
		return kvs_protocol::TIMEOUT;
	}
	//just for suppressing compiler's warnings.
	return 0;
}

kvs_protocol::status
kvs_client::put(kvs_protocol::key key, string value) {
	int connection = next_connection();
	if (connection == -1) {
		return kvs_protocol::RETRY;
	}
	stringstream message;
	message << cs_protocol::PUT;
	message << ' ';
	message << key;
	message << ' ';
	message << value;
	int length = message.str().length();
	length = htonl(length);
	int r = send(connection, &length, 4, 0);
	assert(r == 4);
	r = send(connection, message.str().c_str(), ntohl(length), 0);
	//same as the reason for get
	//assert(r == (int)ntohl(length));
	string r_message;
	if (get_response(connection, r_message)) {
		string r_value;
		switch (get_r_value(r_message, r_value)) {
			case cs_protocol::NOT_PRIMARY:
			case cs_protocol::RETRY:
				primary_server = next_server(primary_server);
				return kvs_protocol::RETRY;
			case cs_protocol::TIMEOUT:
				primary_server = next_server(primary_server);
				return kvs_protocol::TIMEOUT;
			case cs_protocol::OK:
				return kvs_protocol::OK;
		}
	} else {
		update_connection(connection);
		return kvs_protocol::TIMEOUT;
	}
	//just for suppressing compiler's warnings.
	return 0;
}

/*
kvs_protocol::status
kvs_client::testandset(kvs_protocol::key key, string value1, string value2) {
	int connection = next_connection();
	if (connection == -1) {
		return kvs_protocol::RETRY;
	}
	stringstream message;
	message << cs_protocol::TESTANDSET;
	message << ' ';
	message << key;
	message << ' ';
	message << value1;
	message << ' ';
	message << value2;
	int length = message.str().length();
	length = htonl(length);
	int r = send(connection, &length, 4, 0);
	assert(r == 4);
	r = send(connection, message.str().c_str(), ntohl(length), 0);
	//same as the reason for get
	//assert(r == (int)ntohl(length));
	string r_message;
	if (get_response(connection, r_message)) {
		string r_value;
		switch (get_r_value(r_message, r_value)) {
			case cs_protocol::NOT_PRIMARY:
			case cs_protocol::RETRY:
				primary_server = next_server(primary_server);
				return kvs_protocol::RETRY;
			case cs_protocol::TIMEOUT:
				primary_server = next_server(primary_server);
				return kvs_protocol::TIMEOUT;
			case cs_protocol::OK:
				int result;
				result = atoi(r_value.c_str());
				if (result == 1) {
					return kvs_protocol::OK;
				} else {
					return kvs_protocol::TAS_FAIL;
				}
		}
	} else {
		update_connection(connection);
		return kvs_protocol::TIMEOUT;
	}
	//just for suppressing compiler's warnings.
	return 0;
}
*/

bool
kvs_client::get_response(int connection, string &message) {
	char buffer[501];
	unsigned next = 0;
	stringstream result;
	unsigned length;
	int r;
	while (1) {
		r = recv(connection, buffer+next, 500-next, 0);
		//server nerver close connection first.
		//assert(r != 0);
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
	result << (buffer+4);
	while (1) {
		if (next == length+4) {
			break;
		}
		r = recv(connection, buffer, 500, 0);
		//assert(r != 0);
		if (r <= 0) {
			return false;
		}
		next += r;
		buffer[r] = 0;
		result << buffer;
	}
	message = result.str();
	assert(message.length() == length);
	return true;
}

//exactly one return value, just as normal functions do.
cs_protocol::status
kvs_client::get_r_value(string &r_message, string &r_value) {
	stringstream buffer;
	buffer << r_message;
	cs_protocol::status r_status;
	buffer >> r_status;
	buffer >> r_value;
	switch (r_status) {
		case cs_protocol::NOT_PRIMARY:
		case cs_protocol::RETRY:
		case cs_protocol::TIMEOUT:
			assert(r_value.length() == 0);
			break;
		case cs_protocol::OK:
			break;
		default:
			kvs_error("@get_r_value: kvs_client receives an unknown return status %d!\n", r_status);
	}	
	return r_status;
}

int
kvs_client::next_connection() {
	int current_server = primary_server;
	int connection = -1;
	assert((current_server >= 0) && (current_server < server_number));
	pthread_mutex_lock(&sc_mutex);
	if ((connection = server_connections[current_server].second) != -1) {
		pthread_mutex_unlock(&sc_mutex);
		return connection;
	}
	current_server = next_server(current_server);
	for (; current_server != primary_server; current_server = next_server(current_server)) {
		if ((connection = server_connections[current_server].second) != -1) {
			primary_server = current_server;
			break;
		}
	}
	pthread_mutex_unlock(&sc_mutex);
	return connection;
}

int
kvs_client::next_server(int current) {
	return (current + 1) % server_number;
}

void
kvs_client::update_connection(int connection) {
	pthread_mutex_lock(&sc_mutex);
	assert(server_connections[primary_server].second == connection);
	close(connection);
	server_connections[primary_server].second = -1;
	//other servers have more chance to be the new primary even if this dead has been up again.
	primary_server = next_server(primary_server);
	pthread_mutex_unlock(&sc_mutex);
}



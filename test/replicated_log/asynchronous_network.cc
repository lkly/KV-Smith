#include "asynchronous_network.h"
#include "paxos.h"

asynchronous_network::asynchronous_network(map<server_name, server_address> &members, paxos *employer) {
	pthread_mutex_init(&throttler, NULL);
	myemployer = employer;
	mymembers = members;
	map<server_name, server_address>::iterator it;
	for (it = members.begin(); it != members.end(); it++) {
		bzero(&connections[it->first], sizeof(connections[it->first]));
		connections[it->first].sin_family = AF_INET;
		connections[it->first].sin_addr.s_addr = inet_addr((it->second).first.c_str());
		connections[it->first].sin_port = htons(atoi((it->second).second.c_str()));
		r_connections[it->second] = it->first;
	}
	mysock = socket(AF_INET, SOCK_DGRAM, 0);
	if (mysock < 0) {
		kvs_error("@asychronous_network: asynchronous_network can't create mysock!\n");
	}
	struct sockaddr_in server_addr;
	bzero(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	//there may be problems on sending when machines have multiple physical nic.
	//changing s_addr to be the specific nic address will help.
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	string name;
	employer->getname(name);
	assert(name.length() != 0);
	server_addr.sin_port = htons(atoi(members[name].second.c_str()));
	if (bind(mysock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
		kvs_error("@asychronous_network: asynchronous_network can't bind mysock!\n");
	}
}

asynchronous_network::~asynchronous_network() {
	kvs_error("@~asynchronous_network: asynchronous_network's destructor shouldn't be called!\n");
}

void
asynchronous_network::start(asynchronous_network *network) {
	//initialization completed, so it's safe to start working.
	int i = 0;
	for (; i < worker_num; i++) {
		pthread_t th;
		if (pthread_create(&th, NULL, worker, (void *)network) < 0) {
			kvs_error("@start: asynchronous_network starting worker fails!\n");
		}
	}
}

void *
asynchronous_network::worker(void *obj) {
	((asynchronous_network *)obj)->do_work();
	kvs_error("@worker: some worker wants to sleep!\n");
}

void
asynchronous_network::inject(server_name &src, server_name &dest, string &message) {
	pthread_mutex_lock(&throttler);
	int r = sendto(mysock, message.c_str(), message.length(), 0, (struct sockaddr*)(&connections[dest]), sizeof(struct sockaddr_in));
	if (r < 0) {
		kvs_error("@inject: asynchronous_network sending message encoutered an exception!\n");
	}
	assert(r == message.length());
	usleep(10000);
	pthread_mutex_unlock(&throttler);
}

void
asynchronous_network::do_work() {
	char buffer[100];
	while (1) {
		struct sockaddr_in addr;
		socklen_t addr_len;
		bzero(&addr, sizeof(addr));
		addr_len = sizeof(addr);
		//for now, 99 bytes is sufficient, otherwise, should use TCP.
		int r = recvfrom(mysock, (void*)buffer, 99, 0, (struct sockaddr*)(&addr), &addr_len);
		if (r < 0) {
			kvs_error("@do_work: asynchronous_network receiving message encoutered an exception!\n");
		}
		assert(r <= 99);
		if (buffer[0] == 0) {
			serve_me(buffer[1]);
			continue;
		}
		assert(addr_len == sizeof(addr));
		server_address saddr;
		saddr.first = inet_ntoa(addr.sin_addr);
		stringstream ss;
		ss << ntohs(addr.sin_port);
		saddr.second = ss.str();
		server_name source = r_connections[saddr];
		assert(source.length() != 0);
		buffer[r] = 0;
		string message = buffer;
		myemployer->callback(source, message);
	}
}

void
asynchronous_network::serve_me(char service) {
	return;
}




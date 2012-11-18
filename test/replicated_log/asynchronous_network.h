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

using namespace std;

class paxos;

class asynchronous_network {
	public:
		asynchronous_network(map<server_name, server_address> &, paxos *);
		~asynchronous_network();
		static void start(asynchronous_network *);
		void inject(string &, string &, string &);
		static void *worker(void *);
		void do_work();

	private:

		static const int worker_num = 3;

		map<server_name, server_address> mymembers;
		paxos *myemployer;
		pthread_mutex_t throttler;
		int mysock;
		map<server_name, struct sockaddr_in> connections;
		map<server_address, server_name> r_connections;
		void serve_me(char);
};


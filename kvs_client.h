#include <pthread.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <string>
#include "kvs_protocol.h"
#include <vector>
#include <utility>
#include "utils.h"
#include "client-server_protocol.h"
#include <sstream>
#include "common.h"
#include <map>
#include <errno.h>
#include <signal.h>

using namespace std;


class kvs_client {
	public:
		kvs_client(map<server_name, server_address> &);
		~kvs_client();
		kvs_protocol::status get(kvs_protocol::key, string &);
		kvs_protocol::status put(kvs_protocol::key, string);
		void connection_maintain();
		static void *connection_maintainer(void *);

	private:
		map<server_name, server_address> servers;
		int server_number;
		vector<pair<server_name, int> > server_connections;
		int primary_server;
		pthread_mutex_t sc_mutex;
		pthread_t cm;
		bool exiting;

		void try_to_connect(map<server_name, int> &);
		bool connecting_done(int &, server_address &);
		int next_connection();
		bool get_response(int, string &);
		cs_protocol::status get_r_value(string &, string &);
		int next_server(int);
		void update_connection(int);

};


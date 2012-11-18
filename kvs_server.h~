#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <string>
#include <utility>
#include "utils.h"
#include "client-server_protocol.h"
#include <sstream>
#include "common.h"
#include "tiny_table.h"
#include "replicated_log.h"
#include <map>

using namespace std;


class kvs_server {
	public:
		kvs_server(server_name &, server_address &, map<server_name, server_address> &);
		~kvs_server();
		void callback(string &, string &);
		void get_address(server_address &);
		void start();

	private:
		server_name myname;
		server_address myaddress;
		map<server_name, server_address> mymembers;
		bool am_i_primary;
		int client_number;
		pthread_mutex_t cn_mutex;
		pthread_cond_t cn_cv;
		//local_db always be empty at the beginning without snapshot.
		tiny_table local_db;
		int logging_number;
		pthread_mutex_t ln_mutex;
		pthread_cond_t ln_cv;
		pthread_mutex_t log_mutex;
		replicated_log *mylog;

		cs_protocol::status get(int, string &);
		cs_protocol::status put(int, string &);
		bool prologue();
		void epilogue(bool);
		void marshal(string &, cs_protocol::status, string &);
		replicated_log::status log(string, bool);
		void log_prologue();
		void log_epilogue();
		void recover();
		void serve();
		bool to_be_master();
		void redo(string &, int &);
		void heartbeater();
		bool doheartbeat();
		void restart();
};



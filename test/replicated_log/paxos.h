#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <string>
#include <utility>
#include "utils.h"
#include <sstream>
#include "common.h"
#include <map>
#include <vector>
#include "paxos_protocol.h"
#include "log_file.h"
#include "paxos_log.h"
#include "results_buffer.h"

using namespace std;

class asynchronous_network;

class paxos {
	public:
		typedef int status;
		enum xxstatus {
			TIMEOUT = 0x01, OK
		};

		paxos(server_name &, map<server_name, server_address> &, log_file *);
		~paxos();
		void getname(string &);
		paxos::status prepare(int, string &, string &, int);
		paxos::status accept(int, string &, string &, int);
		void learn(int, string &);
		void callback(server_name &, string &);

	private:
		static const int catchup_window = 5;
		results_buffer prepare_result;
		results_buffer accept_result;
		server_name myname;
		vector<server_name> mymembers;
		int majority;
		log_file *wfile;
		paxos_log *mylog;
		int window;
		int first_to_decide;
		map<int, map<int, string> > acceptor_buffer;
		pthread_mutex_t buffer_mutex;
		asynchronous_network *network;

		void init_acceptor_buffer();
		void broadcast(string);
		void check_proposed(vector<string> &, string &);
		bool largerthan(string &, stringstream &);
		bool largerthan(string &, string &);
		void do_prepare(string &, stringstream &);
		void do_accept(string &, stringstream &);
		void do_learn(string &, stringstream &);
		void do_prepared(string &, stringstream &);
		void do_accepted(string &, stringstream &);
		void do_ask(string &, stringstream &);
		void send_to(string, string);
		void passive_catchup(string &, int);
		void prepared(string &, int, string &);
		void prepared(string &, int, string &, string &, string &);
		void learned(string &, int);
		void accepted(string &, int, string &);
		void active_catchup();
		void logged_proposal(int, string, string, string, string &);
		void check_buffer();
};




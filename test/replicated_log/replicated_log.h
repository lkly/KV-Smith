#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <string>
#include <utility>
#include "utils.h"
#include <sstream>
#include "common.h"
#include "paxos.h"
#include <map>

using namespace std;

class replicated_log {
	public:
		typedef int status;
		enum xxstatus {
			OK = 0x01, FAIL, TIMEOUT
		};

		static string special_seq_num;
		static char DELIMITER;

		replicated_log(server_name &, map<server_name, server_address> &);
		~replicated_log();
		replicated_log::status log(string &, bool, int);
		bool next_record(string &, int);
		void reset();

	private:

		bool failure;
		pthread_mutex_t failure_mutex;
		int rwhead;
		pthread_mutex_t rwhead_mutex;
		int movedhead;
		pthread_mutex_t movedhead_mutex;
		int next_try;
		pthread_mutex_t next_try_mutex;
		paxos *mypaxos;
		log_file *myfile;
		server_name myname;

		bool failed();
		void prologue(int &, string &);
		void epilogue(bool, int &);
		void movehead(int &);
		void set_failure();
};





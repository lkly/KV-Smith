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
#include <fstream>
#include <iostream>

using namespace std;

class paxos_log {
	public:
		paxos_log(string &);
		~paxos_log();
		void write(int, string &);
		void read(int, string &);
		int size();

	private:
		static int max_size;
		static int length;
		fstream writer;
		int mytimestamp;
		//reading once.
		vector<string> records;
		pthread_mutex_t records_mutex;
		map<int, int> locations;
		pthread_mutex_t locations_mutex;

		void check_file(string &);
		bool check_copy(fstream &, int &, string &);
		void update_ts(int);
};




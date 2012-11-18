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
		fstream fs;
};




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
#include <iostream>

using namespace std;

class asynchronous_network;

class paxos {
	public:
		typedef int status;
		enum xxstatus {
			TIMEOUT = 0x01, OK
		};

		paxos();
		~paxos();
		void callback(server_name &, string &);
		void getname(string &);
		
};

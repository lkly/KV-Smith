#include <time.h>
#include <unistd.h>
#include <string>
#include <utility>
#include "utils.h"
#include <sstream>
#include "common.h"
#include <map>
#include "log_protocol.h"
#include <iostream>

using namespace std;

class replicated_log {
	public:
		typedef int status;
		enum xxstatus {
			OK = 0x01, FAIL, TIMEOUT
		};

		static const string special_seq_num;

		replicated_log(server_name &, map<server_name, server_address> &);
		~replicated_log();
		status log(string &, bool, int);
		bool next_record(string &, int);
		void reset();

};

const string special_seq_num = "-1 @";

#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <string>
#include <utility>
#include "utils.h"
#include <sstream>
#include "common.h"
#include <map>
#include <iostream>

using namespace std;


class kvs_server {
	public:
		kvs_server();
		~kvs_server();
		void callback(string &, string &);
		void get_address(server_address &);
};



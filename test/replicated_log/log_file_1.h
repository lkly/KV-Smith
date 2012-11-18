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

class log_file {
	public:
		log_file(string &);
		~log_file();
		bool read(int, string &, int);
		int size();
		void write(int, string &);
	
	fstream fs;
	map<int, string> store;
	string name;
};



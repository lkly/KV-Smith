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
#include <sys/time.h>
#include <errno.h>

using namespace std;

class log_file {
	public:
		log_file(string &);
		~log_file();
		bool read(int, string &, struct timespec &);
		int size();
		void write(int, string &);

	private:
		static char delimiter;

		fstream writer;
		pthread_mutex_t writer_mutex;
		fstream reader;
		pthread_mutex_t reader_mutex;
		int w_next;
		int r_next;
		int first_cached;
		pthread_cond_t cache_cv;
		map<int, string> cache;
		pthread_mutex_t cache_mutex;

		void check_file(string &, int &);
		void parse_line(string &, int &, string &);
};






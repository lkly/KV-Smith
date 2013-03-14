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
#include <sys/time.h>
#include <errno.h>

using namespace std;

class results_buffer {
	public:
		results_buffer();
		~results_buffer();
		void register_(int, string &, vector<string> *);
		void unregister(int, string &, unsigned, struct timespec &);
		void fill(int, string &, stringstream &);

	private:
		class context {
			public:
				context(pthread_cond_t *cv_, int th) {cv = cv_; threshold = th;}
				~context() {}
				pthread_cond_t *cv;
				unsigned threshold;
		};

		map<int, map<string, vector<string> *> > myresults;
		map<int, map<string, context *> > mycontexts;
		pthread_mutex_t results_mutex;
};

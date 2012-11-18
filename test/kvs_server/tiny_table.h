#include <map>
#include <string>
#include <pthread.h>
#include "utils.h"
#include "common.h"
#include <assert.h>

using namespace std;

class tiny_table {
	public:
		tiny_table();
		~tiny_table();
		void select(int, string &);
		void update(int, string &);
		void lock(int);
		void unlock(int);

	private:
		map<int, string> internal_store;
		map<int, pthread_mutex_t> locks;
		pthread_mutex_t locks_mutex;
};

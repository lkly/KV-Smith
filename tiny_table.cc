#include "tiny_table.h"

tiny_table::tiny_table() {
	//pthread_mutex_init(&locks_mutex, NULL);
	table_size = 100;
	int i;
	for (i = 0; i < table_size; i++) {
		pthread_mutex_init(&locks[i], NULL);
	}
	string default_ = "@1";
	for (i = 0; i < table_size; i++) {
		internal_store[i] = default_;
	}
}

tiny_table::~tiny_table() {
	kvs_error("@~tiny_table: tiny_table's destructor shouldn't be called!\n");
}

void
tiny_table::select(int id, string &value) {
	//should hold lock id
	value = internal_store[id];
}

void
tiny_table::update(int id, string &value) {
	//should hold lock id
	internal_store[id] = value;
}

void
tiny_table::lock(int id) {
//	pthread_mutex_lock(&locks_mutex);
//	if (locks.find(id) == locks.end()) {
//		pthread_mutex_init(&locks[id], NULL);
//	}
//	pthread_mutex_unlock(&locks_mutex);
	pthread_mutex_lock(&locks[id]);
}

void
tiny_table::unlock(int id) {
//	pthread_mutex_lock(&locks_mutex);
//	assert(locks.find(id) != locks.end());
//	pthread_mutex_unlock(&locks_mutex);
	pthread_mutex_unlock(&locks[id]);
}



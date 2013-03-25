#include "paxos_log.h"

//make all the paxos data holding in memory.
unsigned paxos_log::max_size = 100000;
int paxos_log::length = 50;//80

paxos_log::paxos_log(string &name) {
	string file_name = name + ".log";
	check_file(file_name);
	writer.open(file_name.c_str(), ios::in|ios::out);
	assert(writer.good());
	//assert(records.size() == max_size);
	pthread_mutex_init(&records_mutex, NULL);
}

paxos_log::~paxos_log() {
	kvs_error("@~paxos_log: paxos_log's destructor shouldn't be called!\n");
}

void
paxos_log::write(int number, string &record) {
	//assert(number < max_size);
	pthread_mutex_lock(&records_mutex);
	writer << record;
	writer << '\n';
	writer.flush();
	pthread_mutex_unlock(&records_mutex);
}

int
paxos_log::size() {
	return max_size;
}

void
paxos_log::read(int number, string &record) {
	//use this carefully.
	kvs_error("@read: paxos_log's read shouldn't be called now!\n");
	//assert(number < (int)max_size);
	//pthread_mutex_lock(&records_mutex);
	//record = records[number];
	//pthread_mutex_unlock(&records_mutex);
}

void
paxos_log::check_file(string &file_name) {
	//this is where disk image in.
	//leave it as is now since we are providing empty files.
	return;
}

bool
paxos_log::check_copy(fstream &fs, int &timestamp, string &record) {
	return true;
}

void
paxos_log::update_ts(int ts) {
	return;
}




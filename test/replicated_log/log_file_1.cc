#include "log_file.h"

log_file::log_file(string &name) {
	stringstream ss;
	ss << "log_file_1_";
	ss << name << ".log";
	this->name = name;
	string fname = ss.str();
	fs.open(fname.c_str(), fstream::out);
}

log_file::~log_file() {
}

bool
log_file::read(int id, string &record, int to) {
	if (store.find(id) == store.end()) {
		if (store.size() != id) {
			cout << name << "'s store is inconsistent. " << store.size() << endl;
			exit(1);
		}
		return false;
	}
	record = store[id];
	return true;
}

int
log_file::size() {
	return 0;
}

void
log_file::write(int id, string &record) {
	store[id] = record;
	fs << record << endl;
	fs.flush();
}




#include "paxos_log.h"


paxos_log::paxos_log(string &name) {
	stringstream ss;
	ss << "paxos_log_1_";
	ss << name << ".log";
	string fname;
	fname = ss.str();
	fs.open(fname.c_str(), fstream::out);
}

paxos_log::~paxos_log() {
}

void
paxos_log::write(int id, string &record) {
	fs << id << " " << record << endl;
	fs.flush();
}

void
paxos_log::read(int id, string &record) {
	record = "";
}

int
paxos_log::size() {
	return 5;
}



#include "paxos_log.h"

unsigned paxos_log::max_size = 1000;
int paxos_log::length = 50;//80

paxos_log::paxos_log(string &name) {
	string file_name = name + "_p.log";
	mytimestamp = -1;
	check_file(file_name);
	writer.open(file_name.c_str(), ios::in|ios::out);
	assert(writer.good());
	assert(records.size() == max_size);
	mytimestamp++;
	pthread_mutex_init(&locations_mutex, NULL);
	pthread_mutex_init(&records_mutex, NULL);
}

paxos_log::~paxos_log() {
	kvs_error("@~paxos_log: paxos_log's destructor shouldn't be called!\n");
}

void
paxos_log::write(int number, string &record) {
	//assert(number < max_size);
	number = number%max_size;
	pthread_mutex_lock(&locations_mutex);
	int position = number*2*length + locations[number]*length;
	writer.seekp(position, ios::beg);
	int hashcode = hash(record);
	writer << hashcode;
	writer << ' ';
	writer << mytimestamp;
	writer << ' ';
	writer << record;
	writer << '\n';
	writer.flush();
	mytimestamp++;
	locations[number] = flip(locations[number]);
	pthread_mutex_lock(&records_mutex);
	pthread_mutex_unlock(&locations_mutex);
	records[number] = record;
	pthread_mutex_unlock(&records_mutex);
}

int
paxos_log::size() {
	return max_size;
}

void
paxos_log::read(int number, string &record) {
	assert(number < (int)max_size);
	pthread_mutex_lock(&records_mutex);
	record = records[number];
	pthread_mutex_unlock(&records_mutex);
}

void
paxos_log::check_file(string &file_name) {
	//format designed for easy human-reading.
	//every 78(length-2) bytes sits a hidden guard(\n$)
	//it may be helpful to rewrite timestamps, i.e. ascending from every start.
	fstream fs(file_name.c_str(), ios::in|ios::out);
	assert(fs.is_open());
	unsigned i = 0;
	for (; i < max_size; i++) {
		int position = i * length * 2;
		fs.seekg(position, ios::beg);
		int ts_1;
		string copy_1;
		bool r_1 = check_copy(fs, ts_1, copy_1);
		fs.seekg(position+length, ios::beg);
		int ts_2;
		string copy_2;
		bool r_2 = check_copy(fs, ts_2, copy_2);
		//at most one copy is bad.
		if (r_1) {
			if (r_2) {
				if (ts_1 >= ts_2) {
					records.push_back(copy_1);
					locations[i] = 1;
					update_ts(ts_1);
				} else {
					records.push_back(copy_2);
					locations[i] = 0;
					update_ts(ts_2);
				}
			} else {
				records.push_back(copy_1);
				locations[i] = 1;
				update_ts(ts_1);
			}
		} else if (r_2) {
			records.push_back(copy_2);
			locations[i] = 0;
			update_ts(ts_2);
		} else {
			string dull = "";
			records.push_back(dull);
			locations[i] = 0;
		}
	}
	assert(fs.good());
	fs.close();
}

bool
paxos_log::check_copy(fstream &fs, int &timestamp, string &record) {
	int hashcode;
	fs >> hashcode;
	if (!fs.good()) {
		//space may be overwritten.
		fs.clear();
		return false;
	}
	fs >> timestamp;
	if (!fs.good()) {
		fs.clear();
		return false;
	}
	int pos = fs.tellg();
	fs.seekg(pos+1, ios::beg);
	string r;
	getline(fs, r);
	assert(fs.good());
	if (hash(r) != hashcode) {
		return false;
	}
	record = r;
	return true;
}

void
paxos_log::update_ts(int ts) {
	if (ts > mytimestamp) {
		mytimestamp = ts;
	}
}




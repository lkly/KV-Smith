#include "replicated_log.h"
#include <vector>

//single thread log, so statistics is safe.
int statistics = 0;

//single thread log, so m_call is safe.
int m_call = 0;

int counter[5] = {0, 0, 0, 0, 0};
int counter_100[5] = {0, 0, 0, 0, 0};
int counter_10000[5] = {0, 0, 0, 0, 0};

extern vector<vector<string> > v_data;

pthread_mutex_t test_6_mutex;

void check_test_6() {
	pthread_mutex_lock(&test_6_mutex);
	pthread_mutex_unlock(&test_6_mutex);
}

replicated_log::replicated_log(server_name &name, map<server_name, server_address> &members) {
	pthread_mutex_init(&test_6_mutex, NULL);
}

replicated_log::~replicated_log() {
}

void
check_update(string &record, bool stable) {
	if (!stable) {
		cout << "UPDATE should be in stable state." << endl;
		exit(1);
	}
	stringstream ss;
	ss << record;
	int type;
	int key;
	string value;
	ss >> type;
	ss >> key;
	ss >> value;
	if (counter[key%10] != key/10) {
		cout << "UNMATCHED key " << key << " : " << counter[key%10] << endl;
		exit(1);
	}
	if (value != v_data[key%10][key/10]) {
		cout << "UNMATCHED value" << value << " : " << key << endl;
		exit(1);
	}
	counter[key%10]++;
}

void
check_c_update(string &record, bool stable) {
	if (!stable) {
		cout << "C_UPDATE should be in stable state." << endl;
		exit(1);
	}
	stringstream ss;
	ss << record;
	int type;
	int key;
	int value;
	ss >> type;
	ss >> key;
	ss >> value;
	switch(key) {
		case 100:
			counter_100[value]++;
			break;
		case 10000:
			counter_10000[value]++;
			break;
		default:
			cout << "UNKNOWN key: " << key << endl;
			exit(1);
	}
}

bool set_20 = false;

void
check_p_update(string &record, bool stable) {
	if (!stable) {
		cout << "P_UPDATE should be in stable state." << endl;
		exit(1);
	}
	stringstream ss;
	ss << record;
	int type;
	int key;
	string value;
	ss >> type;
	ss >> key;
	ss >> value;
	if (key == 20 && !set_20 && value == "ice") {
		set_20 = true;
		//ensure holding the lock
		sleep(3);
		return;
	}
	cout << "P_UPDATE should be called only once." << endl;
	exit(1);
}

int called = 0;

replicated_log::status
replicated_log::log(string &record, bool stable, int to) {
	stringstream ss;
	ss << record;
	int log_type;
	ss >> log_type;
	switch (log_type) {
		case log_protocol::HEARTBEAT:
			if (m_call == 66 && called == 1) {
				//wait for test_4 completed.
				sleep(1);
				return replicated_log::FAIL;
			}
			statistics++;
			cout << "STAT: " << statistics << endl;
			sleep(1);
		case log_protocol::BEMASTER:
			if (called == 13 && m_call == 66) {
				pthread_mutex_unlock(&test_6_mutex);
			}
			break;
		case log_protocol::UPDATE:
			if (m_call == 30) {
				cout << "CLEAR STAT" << endl;
				statistics = 0;
			}
			if (m_call < 30) {
				check_update(record, stable);
			}
			if (m_call >= 30 && m_call < 65) {
				check_c_update(record, stable);
			}
			if (m_call == 65) {
				check_p_update(record, stable);
			}
			if (m_call == 66) {
				//wait 1 do log.
				sleep(1);
				m_call++;
				return replicated_log::TIMEOUT;
			}
			if (m_call == 67) {
				m_call++;
				return replicated_log::FAIL;
			}
			m_call++;
			break;
		default:
			cout << "UNKNOWN log type: " << log_type << endl;
			exit(1);
	}
	return replicated_log::OK;
}


bool
replicated_log::next_record(string &record, int to) {
	if (called == 0) {
		called++;
		return false;
	}
	if (called == 1 && m_call == 66) {
		//sufficient time for test_5
		pthread_mutex_lock(&test_6_mutex);
		sleep(4);
	}
	if (called >= 1 && called <= 10 && m_call == 66) {
		stringstream ss;
		ss << log_protocol::UPDATE;
		ss << ' ';
		ss << (999+called);
		ss << ' ';
		ss << (999+called);
		record = ss.str();
		called++;
		return true;
	}
	if (called == 11) {
		stringstream ss;
		ss << log_protocol::UPDATE;
		ss << ' ';
		ss << 20;
		ss << ' ';
		ss << "the";
		record = ss.str();
		called++;
		return true;
	}
	if (called == 12) {
		called++;
		return false;
	}
	cout << "NEXT_RECORD at a wrong time." << endl;
	exit(1);
}


void
replicated_log::reset() {
	if (m_call == 66 && called == 1) {
		return;
	}
	cout << "RESET at a wrong time." << endl;
	exit(1);
}




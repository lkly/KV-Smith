#include "replicated_log.h"


void
print_table(map<server_name, server_address> &table) {
	map<server_name, server_address>::iterator it;
	for (it = table.begin(); it != table.end(); it++) {
		cout << it->first << ": ";
		cout << it->second.first << " ";
		cout << it->second.second << endl;
	}
	cout.flush();
}

replicated_log::replicated_log(server_name &name, map<server_name, server_address> &members) {
}

replicated_log::~replicated_log() {
}

//for log
int next_call = 0;

void
check_to_be_master(string &record, bool stable) {
	cout << "CHECK LOG " << next_call << " TO BE MASTER" << endl;
	if (stable) {
		cout << "LOG " << next_call << " should not be stable"<< endl;
		exit(1);
	}
	stringstream ss;
	ss << log_protocol::BEMASTER;
	if (ss.str() != record) {
		cout << "LOG " << next_call << " has a wrong value: " << record << endl;
		exit(1);
	}
	cout << "CHECK LOG " << next_call << " TO BE MASTER OK" << endl;
}

void
check_mastership_maintain(string &record, bool stable) {
	cout << "CHECK LOG " << next_call << " MAINTAIN MASTERSHIP" << endl;
	if (!stable) {
		cout << "LOG " << next_call << " should be stable"<< endl;
		exit(1);
	}
	stringstream ss;
	ss << log_protocol::HEARTBEAT;
	if (ss.str() != record) {
		cout << "LOG " << next_call << " has a wrong value: " << record << endl;
		exit(1);
	}
	cout << "CHECK LOG " << next_call << " MAINTAIN MASTERSHIP OK" << endl;
}

int test = 1;
//for next_record
int called = 0;
//for reset
int reseted = 0;

replicated_log::status
log_test_1(string &record, bool stable) {
	//start up
	if (next_call == 0 && called == 1 && reseted == 0) {
		check_to_be_master(record, stable);
		next_call++;
		return replicated_log::OK;
	}
	//mastership maintain
	if (next_call < 5 && called == 1 && reseted == 0) {
		check_mastership_maintain(record, stable);
		next_call++;
		return replicated_log::OK;
	}
	//lose mastership
	if (next_call == 5 && called == 1 && reseted == 0) {
		check_mastership_maintain(record, stable);
		cout << "TEST 1 OK" << endl;
		test = 2;
		next_call = 0;
		called = 0;
		reseted = 0;
		return replicated_log::TIMEOUT;
	}
	cout << "LOG is called at a wrong time." << endl;
	exit(1);
}

replicated_log::status
log_test_2(string &record, bool stable) {
	//start up
	if (next_call == 0 && called == 1 && reseted == 1) {
		check_to_be_master(record, stable);
		next_call++;
		return replicated_log::OK;
	}
	//mastership maintain
	if (next_call < 3 && called == 1 && reseted == 1) {
		check_mastership_maintain(record, stable);
		next_call++;
		return replicated_log::OK;
	}
	//lose mastership
	if (next_call == 3 && called == 1 && reseted == 1) {
		check_mastership_maintain(record, stable);
		cout << "TEST 2 OK" << endl;
		test = 3;
		next_call = 0;
		called = 0;
		reseted = 0;
		return replicated_log::FAIL;
	}
	cout << "LOG is called at a wrong time." << endl;
	exit(1);
}

replicated_log::status
log_test_3(string &record, bool stable) {
	//start up
	if (next_call == 0 && called == 4 && reseted == 1) {
		check_to_be_master(record, stable);
		next_call++;
		return replicated_log::OK;
	}
	//mastership maintain
	if (next_call < 3 && called == 4 && reseted == 1) {
		check_mastership_maintain(record, stable);
		next_call++;
		return replicated_log::OK;
	}
	//lose mastership
	if (next_call == 3 & called == 4 && reseted == 1) {
		check_mastership_maintain(record, stable);
		cout << "TEST 3 OK" << endl;
		test = 4;
		next_call = 0;
		called = 0;
		reseted = 0;
		return replicated_log::FAIL;
	}
	cout << "LOG is called at a wrong time."<< endl;
	exit(1);
}

replicated_log::status
log_test_4(string &record, bool stable) {
	if (next_call == 0 && called == 2 && reseted == 1) {
		check_to_be_master(record, stable);
		next_call++;
		return replicated_log::TIMEOUT;
	}
	if (next_call == 1 && called == 3 && reseted == 2) {
		check_to_be_master(record, stable);
		next_call++;
		return replicated_log::OK;
	}
	if (next_call == 2 && called == 3 && reseted == 2) {
		check_mastership_maintain(record, stable);
		cout << "TEST 4 OK" << endl;
		//all tests passed.
		exit(0);
	}
	cout << "LOG is called at a wrong time."<< endl;
	exit(1);
}

replicated_log::status
replicated_log::log(string &record, bool stable, int to) {
	switch(test) {
		case 1:
			return log_test_1(record, stable);
		case 2:
			return log_test_2(record, stable);
		case 3:
			return log_test_3(record, stable);
		case 4:
			return log_test_4(record, stable);
	}
}

bool
next_record_test_1(string &record) {
	if (called == 0 && next_call == 0 && reseted == 0) {
		called = 1;
		return false;
	}
	cout << "NEXT_RECORD is called second time." << endl;
	exit(1);
}

bool
next_record_test_2(string &record) {
	if (called == 0 && next_call == 0 && reseted == 1) {
		called = 1;
		return false;
	}
	cout << "NEXT_RECORD is called second time." << endl;
	exit(1);
}

void
ht_record(string &record) {
	stringstream ss;
	ss << log_protocol::HEARTBEAT;
	record = ss.str();
}

void
bm_record(string &record) {
	stringstream ss;
	ss << log_protocol::BEMASTER;
	record = ss.str();
}

bool
next_record_test_3(string &record) {
	//force an order here.
	if (called == 0 && next_call == 0 && reseted == 1) {
		called++;
		ht_record(record);
		return true;
	}
	if (called == 1 && next_call == 0 && reseted == 1) {
		called++;
		ht_record(record);
		return true;
	}
	if (called == 2 && next_call == 0 && reseted == 1) {
		called++;
		bm_record(record);
		return true;
	}
	if (called == 3 && next_call == 0 && reseted == 1) {
		called++;
		return false;
	}
	cout << "NEXT_RECORD is called at a wrong time." << endl;
	exit(1);
}

bool
next_record_test_4(string &record) {
	if (called == 0 && next_call == 0 && reseted == 1) {
		called++;
		bm_record(record);
		return true;
	}
	if (called == 1 && next_call == 0 && reseted == 1) {
		called++;
		return false;
	}
	if (called == 2 && next_call == 1 && reseted == 2) {
		called++;
		return false;
	}
	cout << "NEXT_RECORD is called at a wrong time." << endl;
	exit(1);
}

bool
replicated_log::next_record(string &record, int to) {
	bool r;
	switch (test) {
		case 1:
			r = next_record_test_1(record);
			break;
		case 2:
			r = next_record_test_2(record);
			break;
		case 3:
			r = next_record_test_3(record);
			break;
		case 4:
			r = next_record_test_4(record);
			break;
	}
	if (r) {
		cout << "GET next record." << endl;
	} else {
		cout << "DON'T get next record."<< endl;
	}
	return r;
}

void
reset_test_1() {
	cout << "RESET shouldn't be called." << endl;
	exit(1);
}

void
reset_test_2() {
	if (reseted == 0 && next_call == 0 && called == 0) {
		reseted = 1;
		return;
	}
	cout << "RESET shouldn't be called more than once." << endl;
	exit(1);
}

void
reset_test_3() {
	if (reseted == 0 && next_call == 0 && called == 0) {
		reseted++;
		return;
	}
	cout << "RESET shouldn't be called more than once." << endl;
	exit(1);
}

void
reset_test_4() {
	if (reseted == 0 && next_call == 0 && called == 0) {
		reseted++;
		return;
	}
	if (reseted == 1 && next_call == 1 && called == 2) {
		reseted++;
		return;
	}
	cout << "RESET is called at a wrong time." << endl;
	exit(1);
}

void
replicated_log::reset() {
	cout << "RESET" << endl;
	switch(test) {
		case 1:
			reset_test_1();
			return;
		case 2:
			reset_test_2();
			return;
		case 3:
			reset_test_3();
			return;
		case 4:
			reset_test_4();
			return;
	}
}



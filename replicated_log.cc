#include "replicated_log.h"

string replicated_log::special_seq_num = "-1 @";
char replicated_log::DELIMITER = '\n';

replicated_log::replicated_log(server_name &name, map<server_name, server_address> &members) {
	//now without snapshot, log always starts from 0.
	rwhead = 0;
	//for parallel logging.
	movedhead = 0;
	failure = false;
	//a different name for each instance.
	//depending on underlying network implementation, there is no need no make each proposal
	//with a brand-new sequence number for differentiating, but from the layer's perspective
	//it is essential.
	//read-only after initialization.
	myname = name;
	next_try = 0;
	pthread_mutex_init(&movedhead_mutex, NULL);
	pthread_mutex_init(&rwhead_mutex, NULL);
	pthread_mutex_init(&failure_mutex, NULL);
	pthread_mutex_init(&next_try_mutex, NULL);
	myfile = new log_file(name);
	//myfile is initialized before calling constructor, so this is safe.
	mypaxos = new paxos(name, members, myfile);
}

replicated_log::~replicated_log() {
	kvs_error("@~replicated_log: replicated_log destructor shouldn't be called!\n");
}

replicated_log::status
replicated_log::log(string &record, bool stable, int timeout) {
	//timeout uses absolute time?
	if (failed()) {
		return FAIL;
	}
	status r_l = OK;
	paxos::status r;
	string proposed;
	int slot_num;
	string seq_num;
	prologue(slot_num, seq_num);
	if (!stable) {
		r = mypaxos->prepare(slot_num, seq_num, proposed, timeout);
		switch (r) {
			case paxos::TIMEOUT:
				epilogue(true, slot_num);
				return FAIL;
			case paxos::OK:
				break;
			default:
				kvs_error("@log: paxos prepare returns an unknown value %d!\n", r);
		}
	}
	string proposal;
	if (!stable && (proposed.length() != 0)) {
		r_l = FAIL;
		proposal = proposed;
	} else {
		proposal = record;
	}
	if (!stable) {
		r = mypaxos->accept(slot_num, seq_num, proposal, timeout);
	} else {
		string str = special_seq_num;
		r = mypaxos->accept(slot_num, str, proposal, timeout);
	}
	switch (r) {
		case paxos::TIMEOUT:
			epilogue(true, slot_num);
			return TIMEOUT;
		case paxos::OK:
			break;
		default:
			kvs_error("@log: paxos accept returns an unknown value %d!\n", r);
	}
	mypaxos->learn(slot_num, proposal);
	if (r_l == OK) {
		epilogue(false, slot_num);
	} else {
		//just set failure.
		set_failure();
	}
	return r_l;
}

bool
replicated_log::next_record(string &record, int timeout) {
	//safe: no one is accessing the log except you.
	assert(movedhead == rwhead);
	bool r = myfile->read(rwhead, record, timeout);
	if (r) {
		rwhead++;
		movedhead++;
	}
	return r;
}

bool
replicated_log::failed() {
	bool r;
	pthread_mutex_lock(&failure_mutex);
	r = failure;
	pthread_mutex_unlock(&failure_mutex);
	return r;
}

void
replicated_log::prologue(int &slot_num, string &seq_num) {
	pthread_mutex_lock(&movedhead_mutex);
	slot_num = movedhead;
	movedhead++;
	pthread_mutex_unlock(&movedhead_mutex);
	pthread_mutex_lock(&next_try_mutex);
	stringstream buffer;
	buffer << next_try;
	buffer << " ";
	buffer << myname;
	seq_num = buffer.str();
	pthread_mutex_unlock(&next_try_mutex);
}

void
replicated_log::epilogue(bool failed, int &slot_num) {
	bool r;
	pthread_mutex_lock(&failure_mutex);
	r = failure;
	if (failed && !failure) {
		failure = true;
	}
	pthread_mutex_unlock(&failure_mutex);
	pthread_mutex_lock(&next_try_mutex);
	//don't want to repeat unnecessary updates. 
	if (failed && !r) {
		next_try++;
	}
	pthread_mutex_unlock(&next_try_mutex);
	if (!failed) {
		pthread_mutex_lock(&rwhead_mutex);
		movehead(slot_num);
		pthread_mutex_unlock(&rwhead_mutex);
	}
	//else: movedhead will be reset by reset().
}

void
replicated_log::movehead(int &slot_num) {
	//hold rwhead lock.
	//and just now.+++
	mv_window[slot_num] = 1;
	while (1) {
		if (mv_window[rwhead+1] != 0) {
			rwhead++;
		} else {
			break;
		}
	}
}

void
replicated_log::set_failure() {
	pthread_mutex_lock(&failure_mutex);
	failure = true;
	pthread_mutex_unlock(&failure_mutex);
}

void
replicated_log::reset() {
	//safe: no one is accessing the log except you.
	assert(failure == true);
	failure = false;
	movedhead = rwhead;
	mv_window.clear();
}

void
replicated_log::skip(int window_size) {
	//safe: same as reset.
	rwhead = rwhead + window_size;
	movehead = rwhead;
}



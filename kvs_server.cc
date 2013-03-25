#include "kvs_server.h"
#include "connection_manager.h"
#include <iostream>

kvs_server::kvs_server(server_name &name, server_address &address, map<server_name, server_address> &members) {
	signal(SIGPIPE, SIG_IGN);
	myname = name;
	myaddress = address;
	mymembers = members;
	am_i_primary = false;
	client_number = 0;
	mylog = new replicated_log(myname, mymembers);
	pthread_mutex_init(&cn_mutex, NULL);
	pthread_cond_init(&cn_cv, NULL);
	pthread_t th;
	if (pthread_create(&th, NULL, connection_manager::getready, (void *)this) < 0) {
		kvs_error("@kvs_server: kvs_server starting connection manager fails!\n");
	}
	pthread_mutex_init(&log_mutex, NULL);
	logging_number = 0;
	pthread_mutex_init(&ln_mutex, NULL);
	pthread_cond_init(&ln_cv, NULL);
	pthread_cond_init(&ln_cv2, NULL);
	struct timeval tv;
	gettimeofday(&tv, NULL);
	expire_time.tv_sec  = tv.tv_sec;//so when start up, it must be expired.
	expire_time.tv_nsec = 0;//we don't use this field, so timing is much coarse-grained.
	map<server_name, server_address>::iterator it;
	int priority;
	priority = 0;
	bool found;
	found = false;
	for (it = mymembers.begin(); it != mymembers.end(); ++it) {
		if (it->first == name) {
			found = true;
			break;
		}
		priority++;
	}
	assert(found);
	delta = priority*2;
}

kvs_server::~kvs_server() {
	kvs_error("@~kvs_server: kvs_server destructor shouldn't be called!\n");
}

void
kvs_server::get_address(server_address &addr) {
	addr = myaddress;
}

void
kvs_server::callback(string &request, string &r_message) {
	//std::cout << "catch request." << std::endl;
	stringstream buffer;
	buffer << request;
	cs_protocol::op_t operation;
	buffer >> operation;
	int key;
	buffer >> key;
	cs_protocol::status r;
	string value;
	if (operation == cs_protocol::GET) {
		r = get(key, value);//may require some asserts here.
		marshal(r_message, r, value);
	} else if (operation == cs_protocol::PUT) {
		buffer >> value;
		r = put(key, value);
		value.clear();
		marshal(r_message, r, value);
	} else {
		kvs_error("@callback: kvs_server receives an unknown operation %d!\n", operation);
	}
}

cs_protocol::status
kvs_server::get(int key, string &value) {
	if (!prologue()) {
		return cs_protocol::NOT_PRIMARY;
	}
	local_db.lock(key);
	local_db.select(key, value);
	local_db.unlock(key);
	epilogue(false);
	return cs_protocol::OK;
}

cs_protocol::status
kvs_server::put(int key, string &value) {
	cs_protocol::status r_status;
	r_status = 0;
	if (!prologue()) {
		return cs_protocol::NOT_PRIMARY;
	}
	local_db.lock(key);
	stringstream record;
	record << log_protocol::UPDATE;
	record << ' ';
	record << key;
	record << ' ';
	record << value;
	log_prologue();
	//should in log_prologue
	struct timespec ts;
	pthread_mutex_lock(&log_mutex);
	ts = expire_time;
	pthread_mutex_unlock(&log_mutex);
	replicated_log::status r = log(record.str(), true, ts);
	if (r == replicated_log::OK) {
		pthread_mutex_lock(&log_mutex);
		renew_lease(1);
		pthread_mutex_unlock(&log_mutex);
	}
	log_epilogue();
	if (r == replicated_log::FAIL) {
		r_status = cs_protocol::RETRY;
	} else if (r == replicated_log::TIMEOUT) {
		r_status = cs_protocol::TIMEOUT;
	} else if (r == replicated_log::OK) {
		r_status = cs_protocol::OK;
		local_db.update(key, value);
	} else {
		kvs_error("@put: kvs_server log returns an unknown status %d!\n", r);
	}
	local_db.unlock(key);
	epilogue(r != replicated_log::OK);
	return r_status;
}

bool
kvs_server::prologue() {
	bool r = false;
	pthread_mutex_lock(&cn_mutex);
	if (am_i_primary) {
		client_number++;
		r = true;
	}
	pthread_mutex_unlock(&cn_mutex);
	return r;
}

void
kvs_server::epilogue(bool clear_primary) {
	pthread_mutex_lock(&cn_mutex);
	if (clear_primary && am_i_primary) {
		am_i_primary = false;
	}
	client_number--;
	//unnecessary to check am_i_primary is false.
	if (client_number == 0) {
		pthread_cond_signal(&cn_cv);
	}
	pthread_mutex_unlock(&cn_mutex);
}

void
kvs_server::marshal(string &r_message, cs_protocol::status status, string &r_value) {
	stringstream buffer;
	buffer << status;
	if (r_value.length() != 0) {
		buffer << ' ';
		buffer << r_value;
	}
	r_message = buffer.str();
}

replicated_log::status
kvs_server::log(string record, bool stable, struct timespec &to) {
	//here is where parallelism in.
	//and now just a plain parallelism-free scheme.
	replicated_log::status r;
	std::cout << "log: " << record << std::endl;
	r = mylog->log(record, stable, to);
	return r;
}

void
kvs_server::renew_lease(int c) {
	//check am_i_primary?
	//a simple scheme:
	//renew lease every 10 successful requests, or a 100ms lease period.
	lease_counter += c;
	expire_time.tv_sec += lease_counter/10;
	lease_counter = lease_counter % 10;
}

void
kvs_server::log_prologue() {
	pthread_mutex_lock(&ln_mutex);
	while (logging_number == window_size) {
		pthread_cond_wait(&ln_cv2, &ln_mutex);
	}
	logging_number++;
	pthread_mutex_unlock(&ln_mutex);
}

void
kvs_server::log_epilogue() {
	pthread_mutex_lock(&ln_mutex);
	if (logging_number == window_size) {
		pthread_cond_signal(&ln_cv2);
	}
	logging_number--;
	if (logging_number == 0) {
		pthread_cond_signal(&ln_cv);
	}
	pthread_mutex_unlock(&ln_mutex);
}

void
kvs_server::start() {
	while (1) {
		recover();
		std::cout << "I'm master." << std::endl;
		serve();
		std::cout << "I'm slave." << std::endl;
	}
}

void
kvs_server::recover() {
	int deltato = 0;
	while (1) {
		string record;
		struct timespec ts;
		//safe, since single-thread recovery.
		ts = expire_time;
		ts.tv_sec += delta;
		if (!mylog->next_record(record, ts)) {
			assert(record.length() == 0);
			if (to_be_master()) {
				pthread_mutex_lock(&cn_mutex);
				assert(client_number == 0);
				am_i_primary = true;
				pthread_mutex_unlock(&cn_mutex);
				mylog->skip(window_size);
				return;
			} else {
				continue;
			}
		}
		std::cout << "redo: " << record << std::endl;
		redo(record, deltato);
	}
}

void
kvs_server::serve() {
	//the next successful logging should happen within NEXTTO+deltato from
	//last BEMASTER logging(as it will since it will be scheduled in this
	//large interval in today's hardware), otherwise, will lose its mastership.
	//and it opens a window to read stale data from different clients' views.
	//first request being put or absolute time check are possible solutions.
	heartbeater();
	restart();
}

bool
kvs_server::to_be_master() {
	//here is where master election for parallelism in.
	//now no parallelism, so single log slot is enough.
	bool success = false;
	stringstream record;
	record << log_protocol::BEMASTER;
	struct timespec ts;
	struct timeval tv;
	gettimeofday(&tv, NULL);
	//master has a 5s initial lease.
	//but we don't want it used up.
	unsigned int sec = tv.tv_sec + 5;
	record << " ";
	record << sec;
	ts.tv_sec  = tv.tv_sec + 3;
	ts.tv_nsec = 0;
	replicated_log::status r = log(record.str(), false, ts);
	switch (r) {
		case replicated_log::OK:
			success = true;
			expire_time.tv_sec = sec;
			break;
		case replicated_log::TIMEOUT:
		case replicated_log::FAIL:
			mylog->reset();
			break;
		default:
			kvs_error("@to_be_master: kvs_server log returns an unknown status %d!\n", r);
	}
	return success;
}

void
kvs_server::redo(string &record, int &deltato) {
	deltato = 0;
	log_protocol::record_t rt;
	stringstream buffer;
	buffer << record;
	buffer >> rt;
	string value;
	switch (rt) {
		case log_protocol::UPDATE:
			//single-threaded recover.
			renew_lease(1);
			int key;
			buffer >> key;
			buffer >> value;
			local_db.update(key, value);
			break;
		case log_protocol::HEARTBEAT:
			renew_lease(12);
			break;
		case log_protocol::BEMASTER:
			//try to eliminate interference to the new master.
			unsigned int sec;
			buffer >> sec;
			expire_time.tv_sec = sec;
			deltato = 3;
			mylog->skip(window_size);
			break;
		default:
			kvs_error("@redo: kvs_server reads an unknown record type %d from log!\n", rt);
	}
}

void
kvs_server::heartbeater() {
	while (1) {
		if (!doheartbeat()) {
			return;
		}
		sleep(1);
		//it may be beneficial to sleep a while here.
		//or consider different timeout configurations.
		//a safe scheme: NEXTTO == DOLOGTO + sleeptime
	}
}

bool
kvs_server::doheartbeat() {
	//heartbeater has lowest priority.
	pthread_mutex_lock(&ln_mutex);
	while (logging_number != 0) {
		pthread_cond_wait(&ln_cv, &ln_mutex);
	}
	pthread_mutex_unlock(&ln_mutex);
	bool success = false;
	stringstream record;
	record << log_protocol::HEARTBEAT;
	struct timespec ts;
	pthread_mutex_lock(&log_mutex);
	ts = expire_time;
	pthread_mutex_unlock(&log_mutex);
	replicated_log::status r = log(record.str(), true, ts);
	switch (r) {
		case replicated_log::OK:
			pthread_mutex_lock(&log_mutex);
			renew_lease(12);//account for scheduling's overhead.
			pthread_mutex_unlock(&log_mutex);
			success = true;
			break;
		case replicated_log::TIMEOUT:
		case replicated_log::FAIL:
			break;
		default:
			kvs_error("@doheartbeat: kvs_server log returns an unknown status %d!\n", r);
	}
	return success;
}

void
kvs_server::restart() {
	pthread_mutex_lock(&cn_mutex);
	am_i_primary = false;
	while (client_number != 0) {
		pthread_cond_wait(&cn_cv, &cn_mutex);
	}
	pthread_mutex_unlock(&cn_mutex);
	lease_counter = 0;
	mylog->reset();
}





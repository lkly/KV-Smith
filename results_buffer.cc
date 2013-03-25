#include "results_buffer.h"

results_buffer::results_buffer() {
	pthread_mutex_init(&results_mutex, NULL);
}

results_buffer::~results_buffer() {
	kvs_error("@~results_buffer: results_buffer's destructor shouldn't be called!\n");
}

void
results_buffer::register_(int slot_num, string &seq_num, vector<string> *results) {
	pthread_mutex_lock(&results_mutex);
	assert(myresults[slot_num].empty());
	myresults[slot_num][seq_num] = results;
	pthread_mutex_unlock(&results_mutex);
}

void
results_buffer::unregister(int slot_num, string &seq_num, unsigned majority, struct timespec &timeout) {
	pthread_mutex_lock(&results_mutex);
	assert((myresults.find(slot_num) != myresults.end()) && (myresults[slot_num].find(seq_num) != myresults[slot_num].end()));
	//why this?
	//if (timeout == 0) {
	//	myresults.erase(slot_num);
	//	pthread_mutex_unlock(&results_mutex);
	//	return;
	//}
	pthread_cond_t slot_cv;
	pthread_cond_init(&slot_cv, NULL);
	context *ct = new context(&slot_cv, majority);
	mycontexts[slot_num][seq_num] = ct;
	while (myresults[slot_num][seq_num]->size() < majority) {
		if (pthread_cond_timedwait(&slot_cv, &results_mutex, &timeout) == ETIMEDOUT) {
			break;
		}
	}
	myresults.erase(slot_num);
	mycontexts.erase(slot_num);
	pthread_mutex_unlock(&results_mutex);
	delete ct;
	pthread_cond_destroy(&slot_cv);
}

void
results_buffer::fill(int slot_num, string &seq_num, stringstream &args) {
	pthread_mutex_lock(&results_mutex);
	if (myresults.find(slot_num) == myresults.end()) {
		pthread_mutex_unlock(&results_mutex);
		return;
	}
	if (myresults[slot_num].find(seq_num) == myresults[slot_num].end()) {
		pthread_mutex_unlock(&results_mutex);
		return;
	}
	vector<string> *vp = myresults[slot_num][seq_num];
	//still need hold the lock since vector may be accessed by paxos thread.
	//no information returned from the sender.
	string fill_str;
	string temp;
	args >> temp;
	if (args.eof()) {
		vp->push_back("");
	} else {
		//must be prepared.
		fill_str = temp;
//		args >> temp;
//		assert(!args.eof());
//		fill_str +=  " ";
//		fill_str += temp;
//		args >> temp;
//		assert(!args.eof());
//		fill_str +=  " ";
//		fill_str += temp;
//		args >> temp;
//		assert(args.eof());
		char readbuff[100];
		args.readsome(readbuff, 99);
		readbuff[args.gcount()] = 0;
		fill_str += readbuff;
		vp->push_back(fill_str);
	}
	if (mycontexts.find(slot_num) == mycontexts.end()) {
		pthread_mutex_unlock(&results_mutex);
		return;
	}
	if (mycontexts[slot_num].find(seq_num) == mycontexts[slot_num].end()) {
		pthread_mutex_unlock(&results_mutex);
		return;
	}
	context *ct = mycontexts[slot_num][seq_num];
	if (vp->size() >= ct->threshold) {
		pthread_cond_signal(ct->cv);
	}	
	pthread_mutex_unlock(&results_mutex);
}




#include "paxos.h"
#include "asynchronous_network.h"

paxos::paxos(server_name &name, map<server_name, server_address> &members, log_file *file) {
	myname = name;
	map<server_name, server_address>::iterator it;
	for (it = members.begin(); it != members.end(); it++) {
		mymembers.push_back(it->first);
	}
	majority = mymembers.size()/2 + 1;
	wfile = file;
	first_to_decide = wfile->size();
	mylog = new paxos_log(name);
	window = mylog->size();
	//neglect this for no effects.
	//init_acceptor_buffer();
	pthread_mutex_init(&buffer_mutex, NULL);
	network = new asynchronous_network(members, this);
	//initialization should in this order.
	asynchronous_network::start(network);
}

paxos::~paxos() {
	kvs_error("@~paxos: paxos destructor shouldn't be called!\n");
}

void
paxos::getname(string &name) {
	name = myname;
}

void
paxos::init_acceptor_buffer() {
	int i;
	for (i = 0; i < window; i++) {
		string proposal;
		mylog->read(i, proposal);
		if (proposal.length() == 0) {
			continue;
		}
		stringstream buffer;
		buffer << proposal;
		int slot_num;
		buffer >> slot_num;
		if (slot_num < first_to_decide) {
			continue;
		}
		string seq_num_part_1, seq_num_part_2;
		buffer >> seq_num_part_1;
		buffer >> seq_num_part_2;
		acceptor_buffer[slot_num][0] = "A";
		acceptor_buffer[slot_num][1] = seq_num_part_1 + " " + seq_num_part_2;
		buffer >> seq_num_part_1;
		if (buffer.eof()) {
			continue;
		}
		buffer >> seq_num_part_2;
		acceptor_buffer[slot_num][2] = seq_num_part_1 + " " + seq_num_part_2;
		string proposal_;
		buffer >> proposal_;
		assert(!buffer.eof());
		acceptor_buffer[slot_num][3] = proposal_;
	}
}

paxos::status
paxos::prepare(int slot_num, string &seq_num, string &proposed, struct timespec &timeout) {
	vector<string> results;
	prepare_result.register_(slot_num, seq_num, &results);
	stringstream message;
	message << paxos_protocol::PREPARE;
	message << " ";
	message << slot_num;
	message << " ";
	message << seq_num;
	broadcast(message.str());
	prepare_result.unregister(slot_num, seq_num, majority, timeout);
	if (results.size() < majority) {
		return TIMEOUT;
	}
	check_proposed(results, proposed);
	return OK;
}

paxos::status
paxos::accept(int slot_num, string &seq_num, string &proposal, struct timespec &timeout) {
	vector<string> results;
	accept_result.register_(slot_num, seq_num, &results);
	stringstream message;
	message << paxos_protocol::ACCEPT;
	message << " ";
	message << slot_num;
	message << " ";
	message << seq_num;
	message << " ";
	message << proposal;
	broadcast(message.str());
	accept_result.unregister(slot_num, seq_num, majority, timeout);
	if (results.size() < majority) {
		return TIMEOUT;
	} else {
		return OK;
	}
}

void
paxos::learn(int slot_num, string &proposal) {
	stringstream message;
	message << paxos_protocol::LEARN;
	message << " ";
	message << slot_num;
	message << " ";
	message << proposal;
	broadcast(message.str());
}

void
paxos::callback(server_name &source, string &message) {
	stringstream buffer;
	buffer << message;
	int type;
	buffer >> type;
	switch (type) {
		case paxos_protocol::PREPARE:
			do_prepare(source, buffer);
			break;
		case paxos_protocol::ACCEPT:
			do_accept(source, buffer);
			break;
		case paxos_protocol::LEARN:
			do_learn(source, buffer);
			break;
		case paxos_protocol::PREPARED:
			do_prepared(source, buffer);
			break;
		case paxos_protocol::ACCEPTED:
			do_accepted(source, buffer);
			break;
		case paxos_protocol::ASK:
			do_ask(source, buffer);
			break;
		default:
			kvs_error("@callback: paxos receives an unknown type %d!\n", type);
	}
}

void
paxos::broadcast(string message) {
	vector<server_name>::iterator it;
	for (it = mymembers.begin(); it != mymembers.end(); it++) {
		network->inject(myname, *it, message);
	}
}

void
paxos::check_proposed(vector<string> &results, string &proposed) {
	//@ can be any symbol.
	string max_seq_num = "-1 @";
	vector<string>::iterator it;
	for (it = results.begin(); it != results.end(); it++) {
		if (it->length() == 0) {
			continue;
		}
		stringstream buffer;
		buffer << *it;
		if (largerthan(max_seq_num, buffer)) {
			continue;
		}
		buffer >> proposed;
		//for assert working.
		buffer >> max_seq_num;
		assert(buffer.eof());
	}
}

bool
paxos::largerthan(string &max_seq_num, stringstream &seq_num_2) {
	stringstream seq_num_1;
	seq_num_1 << max_seq_num;
	int part_1_1, part_1_2;
	string part_2_1, part_2_2;
	seq_num_1 >> part_1_1;
	seq_num_2 >> part_1_2;
	if (part_1_1 > part_1_2) {
		return true;
	}
	seq_num_1 >> part_2_1;
	seq_num_2 >> part_2_2;
	if ((part_1_1 == part_1_2) && (part_2_1 >= part_2_2)) {
		return true;
	}
	stringstream new_seq_num;
	new_seq_num << part_1_2;
	new_seq_num << " ";
	new_seq_num << part_2_2;
	max_seq_num = new_seq_num.str();
	return false;
}

bool
paxos::largerthan(string &seq_num_1, string &seq_num_2) {
	stringstream buffer;
	buffer << seq_num_2;
	return largerthan(seq_num_1, buffer);
}

void
paxos::do_prepare(string &source, stringstream &args) {
	pthread_mutex_lock(&buffer_mutex);
	int slot_num;
	args >> slot_num;
	if (slot_num < first_to_decide) {
		//lagging servers start from prepare, so prepare can help catch-up.
		passive_catchup(source, slot_num);
		return;
	}
	if (slot_num >= (first_to_decide+window)) {
		//discard overflowed slot.
		pthread_mutex_unlock(&buffer_mutex);
		return;
	}
	map<int, string> &persistent_data = acceptor_buffer[slot_num];
	string seq_num_part_1, seq_num_part_2;
	args >> seq_num_part_1;
	args >> seq_num_part_2;
	string proposal;
	string seq_num = seq_num_part_1 + " " + seq_num_part_2;
	if (persistent_data.size() == 0) {
		//it must be non "-1, @" seq_num.
		persistent_data[0] = "A";
		persistent_data[1] = seq_num;
		logged_proposal(slot_num, persistent_data[1], "", "", proposal);
		mylog->write(slot_num, proposal);
		pthread_mutex_unlock(&buffer_mutex);
		prepared(source, slot_num, seq_num);
		return;
	}
	if (acceptor_buffer[slot_num][0] == "L") {
		learned(source, slot_num);
		return;
	}
	if (largerthan(persistent_data[1], seq_num)) {
		pthread_mutex_unlock(&buffer_mutex);
		return;
	}
	if (persistent_data.size() == 2) {
		logged_proposal(slot_num, persistent_data[1], "", "", proposal);
		mylog->write(slot_num, proposal);
		pthread_mutex_unlock(&buffer_mutex);
		prepared(source, slot_num, seq_num);
	} else {
		string seq_num_ac = persistent_data[2];
		string proposed = persistent_data[3];
		logged_proposal(slot_num, persistent_data[1], seq_num_ac, proposed, proposal);
		mylog->write(slot_num, proposal);
		pthread_mutex_unlock(&buffer_mutex);
		prepared(source, slot_num, seq_num, seq_num_ac, proposed);
	}
}

void
paxos::do_accept(string &source, stringstream &args) {
	pthread_mutex_lock(&buffer_mutex);
	int slot_num;
	args >> slot_num;
	if (slot_num < first_to_decide) {
		pthread_mutex_unlock(&buffer_mutex);
		return;
	}
	if (slot_num >= (first_to_decide+window)) {
		//discard overflowed slot.
		pthread_mutex_unlock(&buffer_mutex);
		return;
	}
	map<int, string> &persistent_data = acceptor_buffer[slot_num];
	string seq_num_part_1, seq_num_part_2;
	args >> seq_num_part_1;
	args >> seq_num_part_2;
	string seq_num = seq_num_part_1 + " " + seq_num_part_2;
	string proposed;
	args >> proposed;
	string proposal;
	if (persistent_data.size() == 0) {
		persistent_data[0] = "A";
		//not necessary, just for saving a message.
		//and make np always >= na, so don't require check na then.
		persistent_data[1] = seq_num;
		persistent_data[2] = seq_num;
		persistent_data[3] = proposed;
		logged_proposal(slot_num, seq_num, seq_num, proposed, proposal);
		mylog->write(slot_num, proposal);
		pthread_mutex_unlock(&buffer_mutex);
		accepted(source, slot_num, seq_num);
		return;
	}
	if (acceptor_buffer[slot_num][0] == "L") {
		learned(source, slot_num);
		return;
	}
	//not necessary to set persistent_data[1], do it just for saving a message.
	//and make np always >= na, so don't require check na then.
	if ((persistent_data[1] != seq_num) && largerthan(persistent_data[1], seq_num)) {
		pthread_mutex_unlock(&buffer_mutex);
		return;
	}
	persistent_data[2] = seq_num;
	persistent_data[3] = proposed;
	logged_proposal(slot_num, seq_num, seq_num, proposed, proposal);
	mylog->write(slot_num, proposal);
	pthread_mutex_unlock(&buffer_mutex);
	accepted(source, slot_num, seq_num);
}

void
paxos::do_learn(string &source, stringstream &args) {
	pthread_mutex_lock(&buffer_mutex);
	int slot_num;
	args >> slot_num;
	if (slot_num < first_to_decide) {
		pthread_mutex_unlock(&buffer_mutex);
		return;
	}
	if (slot_num >= (first_to_decide+window)) {
		//lagging servers can have additional storage to hold this,
		//but not now.
		pthread_mutex_unlock(&buffer_mutex);
		return;
	}
	string proposed;
	args >> proposed;
	map<int, string> &persistent_data = acceptor_buffer[slot_num];
	if (persistent_data.size() == 0) {
		persistent_data[0] = "L";
		persistent_data[3] = proposed;
	} else if (persistent_data[0] == "L") {
		pthread_mutex_unlock(&buffer_mutex);
		return;
	} else {
		persistent_data[0] = "L";
		persistent_data[3] = proposed;
	}
	check_buffer();
	pthread_mutex_unlock(&buffer_mutex);
}

void
paxos::do_prepared(string &source, stringstream &args) {
	//active catchup should begin from here.
	int slot_num;
	args >> slot_num;
	string seq_num_part_1, seq_num_part_2;
	args >> seq_num_part_1;
	args >> seq_num_part_2;
	string seq_num = seq_num_part_1 + " " + seq_num_part_2;
	prepare_result.fill(slot_num, seq_num, args);
}

void
paxos::do_accepted(string &source, stringstream &args) {
	int slot_num;
	args >> slot_num;
	string seq_num_part_1, seq_num_part_2;
	args >> seq_num_part_1;
	args >> seq_num_part_2;
	string seq_num = seq_num_part_1 + " " + seq_num_part_2;
	accept_result.fill(slot_num, seq_num, args);
}

void
paxos::do_ask(string &source, stringstream &args) {
	kvs_error("@do_ask: yet to implement do_ask!\n");
}

void
paxos::send_to(string dest, string message) {
	network->inject(myname, dest, message);
}

void
paxos::passive_catchup(string &dest, int slot_num) {
	int catchup_num = min(catchup_window, first_to_decide-slot_num);
	pthread_mutex_unlock(&buffer_mutex);
	struct timespec ts;
	ts.tv_sec = 0;
	ts.tv_nsec = 0;
	int i = 0;
	for (; i < catchup_num; i++) {
		string record;
		bool r = wfile->read(slot_num+i, record, ts);
		if (r) {
			stringstream message;
			message << paxos_protocol::LEARN;
			message << " ";
			message << slot_num;
			message << " ";
			message << record;
			send_to(dest, message.str());
		} else {
			//reading followings may also fail.
			return;
		}
	}
}

void
paxos::prepared(string &dest, int slot_num, string &seq_num) {
	stringstream message;
	message << paxos_protocol::PREPARED;
	message << " ";
	message << slot_num;
	message << " ";
	message << seq_num;
	send_to(dest, message.str());
}

void
paxos::prepared(string &dest, int slot_num, string &seq_num, string &seq_num_ac, string &proposal) {
	stringstream message;
	message << paxos_protocol::PREPARED;
	message << " ";
	message << slot_num;
	message << " ";
	message << seq_num;
	message << " ";
	message << seq_num_ac;
	message << " ";
	message << proposal;
	send_to(dest, message.str());
}

void
paxos::learned(string &dest, int slot_num) {
	string proposal = acceptor_buffer[slot_num][3];
	pthread_mutex_unlock(&buffer_mutex);
	stringstream message;
	message << paxos_protocol::LEARN;
	message << " ";
	message << slot_num;
	message << " ";
	message << proposal;
	send_to(dest, message.str());
}

void
paxos::accepted(string &dest, int slot_num, string &seq_num) {
	stringstream message;
	message << paxos_protocol::ACCEPTED;
	message << " ";
	message << slot_num;
	message << " ";
	message << seq_num;
	send_to(dest, message.str());
}

void
paxos::active_catchup() {
	kvs_error("@active_catchup: yet to implement active_catchup!\n");
}

void
paxos::logged_proposal(int slot_num, string seq_num, string seq_num_ac, string proposed, string &proposal) {
	stringstream buffer;
	buffer << slot_num;
	buffer << " ";
	buffer << seq_num;
	if (seq_num_ac.length() != 0) {
		buffer << " ";
		buffer << seq_num_ac;
	}
	if (proposed.length() != 0) {
		buffer << " ";
		buffer << proposed;
	}
	proposal = buffer.str();
}

void
paxos::check_buffer() {
	int i = 0;
	for (; i < window; i++, first_to_decide++) {
		if (acceptor_buffer[first_to_decide].size() == 0) {
			return;
		}
		if (acceptor_buffer[first_to_decide][0] == "L") {
			wfile->write(first_to_decide, acceptor_buffer[first_to_decide][3]);
			//writing to paxos log
			string proposal;
			logged_proposal(first_to_decide, "D", "", acceptor_buffer[first_to_decide][3], proposal);
			mylog->write(first_to_decide, proposal);
			acceptor_buffer.erase(first_to_decide);
		} else {
			return;
		}
	}
}



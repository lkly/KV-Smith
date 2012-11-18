#include "kvs_server.h"
#include "connection_manager.h"

const char *server_addr = "127.0.0.1 10000";

int data_size = 9;

const char *data[] = {
	"abc", "abcdef", "abcdefghi",
	"123", "123456", "123456789",
	"1a2b3d", "xyz123", "000000",
};

kvs_server::kvs_server() {
}

kvs_server::~kvs_server() {
}

bool ok[5] = {false, false, false, false, false};

int counters[5] = {0, 0, 0, 0, 0};

pthread_mutex_t ok_mutex;

void
kvs_server::callback(string &request, string &result) {
	stringstream ss;
	ss << request;
	int id;
	ss >> id;
	string message;
	ss >> message;
	if (id >= 5 || id < 0) {
		cout << "WRONG id " << id << endl;
		exit(1);
	}
	if (message != data[counters[id]]) {
		cout << "MESSAGE " << message << " is not same as data " << counters[id] << endl;
		exit(1);
	}
	cout << id << "'s data " << counters[id] << "ok." << endl;
	result = data[counters[id]];
	if (counters[id] == 8) {
		pthread_mutex_lock(&ok_mutex);
		ok[id] = true;
		pthread_mutex_unlock(&ok_mutex);
		return;
	}
	counters[id]++;
}

void
kvs_server::get_address(server_address &addr) {
	stringstream ss;
	ss << server_addr;
	ss >> addr.first;
	ss >> addr.second;
}

void
check_all() {
	while(1) {
		pthread_mutex_lock(&ok_mutex);
		bool r = true;
		int i = 0;
		for (; i < 5; i++) {
			r &= ok[i];
		}
		if (r) {
			//wait data returned to peers.
			sleep(2);
			cout << "TEST ok" << endl;
			exit(0);
		}
		pthread_mutex_unlock(&ok_mutex);
		sleep(3);
	}
}


int
main() {
	kvs_server *server = new kvs_server();
	pthread_mutex_init(&ok_mutex, NULL);
	pthread_t th;
	if (pthread_create(&th, NULL, connection_manager::getready, (void *)server) < 0) {
		cout << "CAN'T start cm." << endl;
		exit(1);
	}
	check_all();
	return 0;
}




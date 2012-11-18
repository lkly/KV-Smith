#include "paxos.h"
#include "asynchronous_network.h"

const char *servers[] = {
	"A 127.0.0.1 20000",
	"B 127.0.0.1 20001",
	"C 127.0.0.1 20002",
};

map<server_name, server_address> table;
map<string, string> col;

pthread_mutex_t col_mutex;

asynchronous_network *net;

void
cons_table(map<server_name, server_address> &table) {
	int i = 0;
	for (; i < 3; i++) {
		stringstream ss;
		ss << servers[i];
		string name;
		string ip_addr;
		string port;
		ss >> name;
		ss >> ip_addr;
		ss >> port;
		table[name] = make_pair(ip_addr, port);
	}
}


paxos::paxos() {

}

paxos::~paxos() {

}

void
paxos::callback(server_name &src, string &mes) {
	pthread_mutex_lock(&col_mutex);
	col[mes] = mes;
	stringstream ss;
	ss << mes;
	string name;
	ss >> name;
	if (src != name) {
		cout << "UNKNOWN peer." << endl;
		exit(1);
	}
	pthread_mutex_unlock(&col_mutex);
}

void
paxos::getname(string &name) {
	name = "A";
}

void*
thread_1(void *pt) {
	int i = 0;
	string A = "A";
	string B = "B";
	string C = "C";
	for (; i < 10; i++) {
		stringstream ss;
		ss << A;
		ss << ' ';
		ss << i;
		string str = ss.str();
		net->inject(A, A, str);
		net->inject(A, B, str);
		net->inject(A, C, str);
		cout << "A->B: " << i << endl;
		cout << "A->C: " << i << endl;
		cout << "A->A: " << i << endl;
		usleep(100000);
	}
	pthread_exit(NULL);
}

void*
thread_2(void *pt) {
	int i = 10;
	string A = "A";
	string B = "B";
	string C = "C";
	for (; i < 20; i++) {
		stringstream ss;
		ss << A;
		ss << ' ';
		ss << i;
		string str = ss.str();
		net->inject(A, C, str);
		net->inject(A, B, str);
		net->inject(A, A, str);
		cout << "A->B: " << i << endl;
		cout << "A->A: " << i << endl;
		cout << "A->C: " << i << endl;
		usleep(100000);
	}
	pthread_exit(NULL);
}

void
check() {
	int i = 0;
	for (; i < 20; i++) {
		stringstream ss;
		ss << "B";
		ss << ' ';
		ss << i;
		if (col[ss.str()] != ss.str()) {
			cout << "WRONG message(" << ss.str() << "): " << col[ss.str()] << endl;
			exit(1);
		}
	}
	i = 0;
	for (; i < 20; i++) {
		stringstream ss;
		ss << "C";
		ss << ' ';
		ss << i;
		if (col[ss.str()] != ss.str()) {
			cout << "WRONG message(" << ss.str() << "): " << col[ss.str()] << endl;
			exit(1);
		}
	}
	i = 0;
	for (; i < 20; i++) {
		stringstream ss;
		ss << "A";
		ss << ' ';
		ss << i;
		if (col[ss.str()] != ss.str()) {
			cout << "WRONG message(" << ss.str() << "): " << col[ss.str()] << endl;
			exit(1);
		}
	}
}


int main() {
	pthread_mutex_init(&col_mutex, NULL);
	cons_table(table);
	paxos *mypaxos = new paxos();
	net = new asynchronous_network(table, mypaxos);
	asynchronous_network::start(net);
	sleep(2);
	pthread_t th_0;
	pthread_t th_1;
	pthread_create(&th_0, NULL, thread_1, NULL);
	pthread_create(&th_1, NULL, thread_2, NULL);
	pthread_join(th_0, NULL);
	pthread_join(th_1, NULL);
	//server may send faster.
	sleep(1);
	check();
	cout << "TEST S OK" << endl;
	return 0;
}




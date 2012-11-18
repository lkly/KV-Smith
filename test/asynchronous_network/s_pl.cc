#include "paxos_log.h"
#include <iostream>


using namespace std;

int main () {
	string name = "A";
	paxos_log *pl = new paxos_log(name);
	if (pl->size() != 5) {
		cout << "bad size: " << pl->size() << endl;
	}
	int i = 0;
	for (; i < 5; i++) {
		string rd;
		pl->read(i, rd);
		if (rd.length() != 0) {
			cout << "bad length: " << rd.length() << endl;
		}
	}
	i = 0;
	for (; i < 10; i++) {
		stringstream ss;
		ss << i;
		string str = ss.str();
		pl->write(i, str);
	}
}

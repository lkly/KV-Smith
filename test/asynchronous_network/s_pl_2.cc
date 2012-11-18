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
		stringstream ss;
		ss << i+5;
		string rd;
		pl->read(i, rd);
		if (rd != ss.str()) {
			cout << "bad record: " << rd << endl;
		}
	}
	i = 0;
	for (; i < 1; i++) {
		stringstream ss;
		ss << i+10;
		string str = ss.str();
		pl->write(i, str);
	}
}

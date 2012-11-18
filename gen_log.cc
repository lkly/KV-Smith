#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <stdlib.h>

using namespace std;

int main(int argc, char *argv[]) {
	if (argc != 2) {
		cout << "REQUIRE 2 args." << endl;
		exit(1);
	}
	stringstream fname;
	fname << argv[1];
	fname << "_p.log";
	fstream fs(fname.str().c_str(), ios::out);
	stringstream ss;
	int i = 0;
	for (; i < 48; i++) {
		ss << ' ';
	}
	ss << '\n';
	ss << '$';
	i = 0;
	for (; i < 10; i++) {
		fs << ss.str();
	}
	fs.flush();
	fs.close();
}

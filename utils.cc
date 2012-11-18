#include "utils.h"

void
kvs_error(const char *format, ...) {
	va_list args;
	va_start(args, format);
	vprintf(format, args);
	va_end(args);
	fflush(stdout);
	exit(1);
}

int
max(int a, int b) {
	return (a >= b) ? a : b;
}

int
min(int a, int b) {
	return (a <= b) ? a : b;
}

//Java's string hash function implementation.
//str[0]*31^(n-1) + str[1]*31^(n-2) + ... + str[n-1]
int
hash(string &str) {
	int r = 0;
	unsigned i = 0;
	for (; i < str.length(); i++) {
		r = (r << 5) - r + str[i];
	}
	return r;
}

int
flip(int n) {
	return (n+1)%2;
}



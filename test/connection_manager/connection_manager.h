#include <pthread.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <string>
#include <vector>
#include <utility>
#include "utils.h"
#include <sstream>
#include "common.h"

using namespace std;

class kvs_server;

class connection_manager {
	public:
		connection_manager(kvs_server *);
		~connection_manager();
		void start();

		static void *getready(void *);
		static void *worker(void *);
		static bool get_request(int, string &);
		static bool return_result(int, string &);

	private:
		kvs_server *employer;

		class context {
			public:
				context(kvs_server *emp, int fd) {employer = emp; worker_fd = fd;}
				~context() {}

				kvs_server *employer;
				int worker_fd;
		};


};


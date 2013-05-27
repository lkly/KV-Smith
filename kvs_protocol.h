class kvs_protocol {
	public:
		typedef int status;
		typedef int key;
		enum xxstatus {
			OK = 0x1, RETRY, TIMEOUT, TAS_FAIL
		};
};


class cs_protocol {
	public:
		typedef int op_t;
		enum opcode {
			GET = 0x01, PUT
		};
		typedef int status;
		enum xxstatus {
			OK = 0x01, NOT_PRIMARY, RETRY,
			// when the client side's timeout is larger than the server side's
			TIMEOUT
		};
};

class log_protocol {
	public:
		typedef int record_t;
		enum record_type {
			UPDATE = 0x1, HEARTBEAT, BEMASTER, Con_UPDATE,
		};
		static const char DELIMITER = '\n';
		//invariant: DOLOGTO <= NEXTTO (!= protect against clock drift) and DOLOGTO < CSTO
		enum timeouts {
			DOLOGTO = 3, DONEXTTO = 4
		};
};

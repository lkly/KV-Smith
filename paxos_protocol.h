class paxos_protocol{
	public:
		enum xxtype {
			PREPARE = 0x01, ACCEPT, LEARN, PREPARED, ACCEPTED, ASK
		};
};

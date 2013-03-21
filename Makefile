.PHONY: default clean all depend eval

JUNK =  *.o *.core core.*
PRODUCTS = kvs_server kvs_client
EVAL = eval-s eval-c
LIBS = -lpthread
CC = g++
LD = g++
CFLAGS = -Wall -g -O2

SRCS_SERVER = kvs_server.cc connection_manager.cc tiny_table.cc\
              replicated_log.cc log_file.cc paxos.cc paxos_log.cc results_buffer.cc\
              asynchronous_network.cc s_main.cc eval-s.cc
#gen_log.cc
SRCS_UTILS = utils.cc

SRCS_CLIENT = kvs_client.cc c_main.cc eval-c.cc

SRCS = $(SRCS_SERVER) $(SRCS_CLIENT) $(SRCS_UTILS)

OBJS_SERVER = $(SRCS_SERVER:.cc=.o)

OBJS_CLIENT = $(SRCS_CLIENT:.cc=.o)

OBJS_UTILS = $(SRCS_UTILS:.cc=.o)

default:
	@echo PLZ check README for usage

clean:
	rm -f $(JUNK) $(PRODUCTS)

all: $(PRODUCTS)

eval: $(EVAL)
	chmod +x eval-c.py
	chmod +x eval-s.py
	chmod +x eval.py
	chmod +x prepare.py

depend:
	makedepend -Y. $(SRCS)

%.o: %.cc
	$(CC) $(CFLAGS) -o $@ -c $<

kvs_client: $(OBJS_CLIENT) $(OBJS_UTILS)
	$(LD) -o $@ $^ $(LIBS)

kvs_server: $(OBJS_SERVER) $(OBJS_UTILS)
	$(LD) -o $@ $^ $(LIBS)

eval-c: $(OBJS_CLIENT)
	$(LD) -o $@ $^ $(LIBS)

eval-s: $(OBJS_SERVER)
	$(LD) -o $@ $^ $(LIBS)

gen_log: gen_log.o
	$(LD) -o $@ $<

# DO NOT DELETE

kvs_server.o: kvs_server.h utils.h client-server_protocol.h common.h
kvs_server.o: tiny_table.h replicated_log.h paxos.h paxos_protocol.h
kvs_server.o: log_file.h paxos_log.h results_buffer.h log_protocol.h
kvs_server.o: connection_manager.h
connection_manager.o: connection_manager.h utils.h common.h kvs_server.h
connection_manager.o: client-server_protocol.h tiny_table.h replicated_log.h
connection_manager.o: paxos.h paxos_protocol.h log_file.h paxos_log.h
connection_manager.o: results_buffer.h log_protocol.h
tiny_table.o: tiny_table.h utils.h common.h
replicated_log.o: replicated_log.h utils.h common.h paxos.h paxos_protocol.h
replicated_log.o: log_file.h paxos_log.h results_buffer.h
log_file.o: log_file.h utils.h common.h
paxos.o: paxos.h utils.h common.h paxos_protocol.h log_file.h paxos_log.h
paxos.o: results_buffer.h asynchronous_network.h
paxos_log.o: paxos_log.h utils.h common.h
results_buffer.o: results_buffer.h utils.h common.h
asynchronous_network.o: asynchronous_network.h utils.h common.h paxos.h
asynchronous_network.o: paxos_protocol.h log_file.h paxos_log.h
asynchronous_network.o: results_buffer.h
s_main.o: kvs_server.h utils.h client-server_protocol.h common.h tiny_table.h
s_main.o: replicated_log.h paxos.h paxos_protocol.h log_file.h paxos_log.h
s_main.o: results_buffer.h log_protocol.h
kvs_client.o: kvs_client.h kvs_protocol.h utils.h client-server_protocol.h
kvs_client.o: common.h
c_main.o: kvs_client.h kvs_protocol.h utils.h client-server_protocol.h
c_main.o: common.h
utils.o: utils.h

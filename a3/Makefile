##########################################################################
# CSC469 Winter 2013, A3 Makefile ########################################
##########################################################################

SHELL = bash
ME := $(shell who am i | cut -f1 -d' ')
KILL_CMD := killall -v -u $(ME)
QUEUES = $(shell ipcs -q | grep $(ME) | cut -f2 -d' ')

CC = gcc
CFLAGS = -Wall -g -DDEBUG -DDBG_TCP # -DDBG_UDP -DDBG_RCV -DUSE_LOCN_SERVER
SERVER_BIN = chatserver 
SERVER_OBJS = server_util.o server_main.o

CHATCLIENT_BIN = chatclient
CLIENT_BIN = $(CHATCLIENT_BIN) receiver
CLIENT_OBJS = client_main.o client_util.o 
RECVR_OBJS = client_recv.o client_util.o

TCP_PORT = 49155
UDP_PORT = $(TCP_PORT)
TEST_NAME = testy_mctesterson
HOST_NAME = localhost

##########################################################################

all: $(SERVER_BIN) $(CLIENT_BIN)

chatserver: $(SERVER_OBJS) 
	$(CC) $(CFLAGS) $(SERVER_OBJS) -o $(SERVER_BIN)

server_util.o: server_util.c server.h defs.h
server_main.o: server_main.c defs.h server.h

chatclient: $(CLIENT_OBJS) 
	$(CC) $(CFLAGS) $(CLIENT_OBJS) -o $(CHATCLIENT_BIN)

client_util.o: client_util.c client.h defs.h
client_main.o: client_main.c client.h defs.h 

receiver: $(RECVR_OBJS)
	$(CC) $(CFLAGS) $(RECVR_OBJS) -o receiver

clean:
	rm -f *.o $(SERVER_BIN) $(CLIENT_BIN) core

###########################################################################
# testing targets (for running locally)
###########################################################################

run-server: $(SERVER_BIN)
	./$(SERVER_BIN) -t $(TCP_PORT) -u $(UDP_PORT) &
	
run-client: $(CHATCLIENT_BIN)
	./$(CHATCLIENT_BIN) \
	  -h $(HOST_NAME) -t $(TCP_PORT) -u $(UDP_PORT) -n $(TEST_NAME)

run-both:
	make run-server && sleep 2 && make run-client

kill-server:
	$(KILL_CMD) $(SERVER_BIN)

kill-client:
	$(KILL_CMD) $(CHATCLIENT_BIN)

kill-both:
	make kill-server ; make kill-client

clean-queues:
	ipcrm -q $(QUEUES)


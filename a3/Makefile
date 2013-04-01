##########################################################################
# CSC469 Winter 2013, A3 Makefile ########################################
##########################################################################

SHELL = bash
ME := $(shell who am i | cut -f1 -d' ')
KILL_CMD := killall -v -u $(ME)
QUEUES = $(shell ipcs -q | grep $(ME) | cut -f2 -d' ' | sed 's/^/-q /')

CC = gcc
CFLAGS = -Wall -g -DDEBUG -DDBG_LOC -DDBG_ACTIVE -DUSE_LOCN_SERVER -DDBG_RCV #-DDBG_TCP -DDBG_UDP
SERVER_BIN = chatserver 
SERVER_OBJS = server_util.o server_main.o

RECEIVER_BIN = receiver
CHATCLIENT_BIN = chatclient
CLIENT_BIN = $(CHATCLIENT_BIN) $(RECEIVER_BIN)
CLIENT_OBJS = client_main.o client_util.o 
RECVR_OBJS = client_recv.o client_util.o

TCP_PORT = 49155
UDP_PORT = $(TCP_PORT)
NOW := $(shell echo $$$$)
TEST_NAME = testy-$(NOW)
HOST_NAME = localhost

##########################################################################

all: $(SERVER_BIN) $(CLIENT_BIN)

$(SERVER_BIN): $(SERVER_OBJS) 
	$(CC) $(CFLAGS) $(SERVER_OBJS) -o $(SERVER_BIN)

server_util.o: server_util.c server.h defs.h
server_main.o: server_main.c defs.h server.h

$(CHATCLIENT_BIN): $(CLIENT_OBJS) $(RECEIVER_BIN)
	$(CC) $(CFLAGS) $(CLIENT_OBJS) -o $(CHATCLIENT_BIN)

client_util.o: client_util.c client.h defs.h
client_main.o: client_main.c client.h defs.h 

$(RECEIVER_BIN): $(RECVR_OBJS)
	$(CC) $(CFLAGS) $(RECVR_OBJS) -o $(RECEIVER_BIN)

clean:
	rm -f *.o $(SERVER_BIN) $(CLIENT_BIN) core

###########################################################################
# testing targets (for running locally)
###########################################################################

run-server: $(SERVER_BIN)
	rm -f server.log
	./$(SERVER_BIN) -t $(TCP_PORT) -u $(UDP_PORT) -r room.cfg -f server.log &
	
run-client: $(CHATCLIENT_BIN)
	./$(CHATCLIENT_BIN) \
	  -h $(HOST_NAME) -t $(TCP_PORT) -u $(UDP_PORT) -n $(TEST_NAME)

run-both:
	make run-server && sleep 2 && make run-client

kill-server:
	$(KILL_CMD) $(SERVER_BIN)

kill-client:
	$(KILL_CMD) $(CHATCLIENT_BIN) ; $(KILL_CMD) $(RECEIVER_BIN)

kill-both:
	make kill-server ; make kill-client

clean-queues:
	ipcrm $(QUEUES)

###########################################################################
# cdf testing target -- uses wget and awk to connect to the server given
# in http://www.cdf.toronto.edu/~csc469h/winter/chatserver.txt WE STILL
# NEED TO GIVE OUR CLIENT THE ABILITY TO DO THIS ON ITS OWN -- THIS IS JUST
# FOR TESTING WITH ANGELA'S SERVER
###########################################################################

cdf-run-client: $(CHATCLIENT_BIN)
	./client-to-cdf.sh $(TEST_NAME)


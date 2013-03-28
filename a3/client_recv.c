/*
 *   CSC469 Fall 2010 A3
 *  
 *      File:      client_recv.c 
 *      Author:    Angela Demke Brown
 *      Version:   1.0.0
 *      Date:      17/11/2010
 *   
 * Please report bugs/comments to demke@cs.toronto.edu
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/time.h>
#include <sys/types.h>

#include "client.h"

/*
 * Timeout on select() call, after this many seconds of waiting for chat
 * messages from the server we will check if there is anything in the
 * queue from client_main
 */ 
#define SELECT_INTERVAL_S 2

/* TODO: push this up to client.h? */
#define err_quit(...) \
    fprintf(stderr, "ERROR: "); fprintf(stderr, ##__VA_ARGS__); \
    exit(1);


int listen_sock;
static char *option_string = "f:";

/* For communication with chat client control process */
int ctrl2rcvr_qid;
char ctrl2rcvr_fname[MAX_FILE_NAME_LEN];


void usage(char **argv) {
  printf("usage:\n");
  printf("%s -f <msg queue file name>\n",argv[0]);
  exit(1);
}


void open_client_channel(int *qid) {

    /* Get messsage channel */
    key_t key = ftok(ctrl2rcvr_fname, 42);

    if ((*qid = msgget(key, 0400)) < 0) {
	perror("open_channel - msgget failed");
	fprintf(stderr,"for message channel ./msg_channel\n");

	/* No way to tell parent about our troubles, unless/until it 
	 * wait's for us.  Quit now.
	 */
	exit(1);
    }

    return;
}

void send_error(int qid, u_int16_t code)
{
  /* Send an error result over the message channel to client control process */
  msg_t msg;

  msg.mtype = CTRL_TYPE;
  msg.body.status = RECV_NOTREADY;
  msg.body.value = code;

  if (msgsnd(qid, &msg, sizeof(struct body_s), 0) < 0) {
    perror("send_error msgsnd");
  }
							 
}

void send_ok(int qid, u_int16_t port)
{
  /* Send "success" result over the message channel to client control process */
  msg_t msg;

  msg.mtype = CTRL_TYPE;
  msg.body.status = RECV_READY;
  msg.body.value = port;

  if (msgsnd(qid, &msg, sizeof(struct body_s), 0) < 0) {
    perror("send_ok msgsnd");
  } 

}

/* Macros for generating a port number in the IANA-suggested
 * ephemeral range of 49152 - 65535
 */
#define RAND_PORT_HI 65535
#define RAND_PORT_LO 49152
#define RAND_PORT_WIDTH ( (RAND_PORT_HI) - (RAND_PORT_LO) )
#define RAND_PORT() ( (int) ( (1.0 * rand() / RAND_MAX) * RAND_PORT_WIDTH \
    + RAND_PORT_LO ) )
#define PORT_STR_LEN 6

void init_receiver()
{

    struct addrinfo hints, *addr_result, *r;
    int ret, err_save, port;
    char str_port[PORT_STR_LEN];


    /********************************************
     *  Make sure we can talk to parent (client control process) 
     */
    printf("Trying to open client channel\n");
    open_client_channel(&ctrl2rcvr_qid);

    /********************************************
     * Initialize UDP socket for receiving chat messages.
     */
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    port = RAND_PORT();
    sprintf(str_port, "%d", port);

    if ((ret = getaddrinfo(NULL, str_port, &hints, &addr_result))
        != 0) {
        err_save = ret;
        debug_sub_print(DBG_UDP, "%s: getaddrinfo: %s\n",
            __func__, gai_strerror(ret));
    } else {
        /* Try to bind one of the results */
        for (r = addr_result; r != NULL; r = r->ai_next) {
            if ((listen_sock = socket(r->ai_family, r->ai_socktype,
                r->ai_protocol)) == -1) {
                err_save = errno;
                debug_sub_print(DBG_UDP, "%s: socket: %s\n", __func__,
                    strerror(err_save));
                continue;
            }

            if ((bind(listen_sock, r->ai_addr, r->ai_addrlen)) == -1) {
                err_save = errno;
                debug_sub_print(DBG_UDP, "%s: bind: %s\n", __func__,
                    strerror(err_save));
                continue;
            }

            break;
        }
    }

    /********************************************
     * Tell parent the port number if successful, or failure code if not. 
     * Use the send_error and send_ok functions
     */

    if (r == NULL) {
        debug_sub_print(DBG_RCV, "%s: failed to bind to a socket\n",
            __func__);
        send_error(ctrl2rcvr_qid, err_save);
        err_quit("%s: failed to bind to a socket\n", __func__);
    }

    send_ok(ctrl2rcvr_qid, port);

}




/* Function to deal with a single message from the chat server */

void handle_received_msg(char *buf)
{
/*
 * Some trickery is employed here. FMT_FMT is the string we're going
 * to use to build a format string which we use to print chat
 * messages. FMT_LEN is the max length of FMT_FMT _after_ sprintf'ing
 * MAX_MEMBER_NAME_LEN (assuming a 2-digit MAX_MEMBER_LEN -- its value
 * is 24 right now) and text_len (assuming no more than 4 digits,
 * based on the value of 2048 for MAX_MSG_LEN) into it.
 *
 * No newline because there is one on the end of the messages we get
 * from clients TODO: enusure that received messages have a newline
 */
#define FMT_FMT "%%%ds | %%.%ds"
#define FMT_LEN 17

    struct chat_msghdr *chat = (struct chat_msghdr *) buf;
    char fmt[FMT_LEN];
    int text_len = htons(chat->msg_len) - sizeof(struct chat_msghdr);

    sprintf(fmt, FMT_FMT, MAX_MEMBER_NAME_LEN, text_len);
    /* fmt looks something like this now: "%##s | %.####s\n" */
    printf(fmt, chat->sender.member_name, chat->msgdata);

}



/* Main function to receive and deal with messages from chat server
 * and client control process.  
 *
 * You may wish to refer to server_main.c for an example of the main 
 * server loop that receives messages, but remember that the client 
 * receiver will be receiving (1) connection-less UDP messages from the 
 * chat server and (2) IPC messages on the from the client control process
 * which cannot be handled with the same select()/FD_ISSET strategy used 
 * for file or socket fd's.
 */
void receive_msgs()
{
    int bytes;
    struct timeval to; /* timeout for select */
    fd_set readfds;
    char *buf = (char *)malloc(MAX_MSG_LEN);
    msg_t *msg = (msg_t *)buf;
  
    if (buf == 0) {
        printf("Could not malloc memory for message buffer\n");
        exit(1);
    }


    while(TRUE) {

        /****************************************
         * Wait for a bit on a chat message...
         */
        to.tv_sec = SELECT_INTERVAL_S;
        FD_ZERO(&readfds);
        FD_SET(listen_sock, &readfds);

        // TODO: check return val
        select(listen_sock+1, &readfds, NULL, NULL, &to);

        if (FD_ISSET(listen_sock, &readfds)) {
            debug_sub_print(0, "%s: Chat message from server "
                "requires processing\n", __func__);

            bytes = recv(listen_sock, buf, MAX_MSG_LEN, 0);
            handle_received_msg(buf);

        } else {
            debug_sub_print(0, "%s: No messages from server\n",
                __func__);
        }

        /****************************************
         * ... check for messages from client_main...
         */
        bytes = msgrcv(ctrl2rcvr_qid, buf, MAX_MSG_LEN,
            RECV_TYPE, IPC_NOWAIT);

        if (bytes == -1) {
            // EAGAIN and ENOMSG are expected
            if ((errno != EAGAIN) && (errno != ENOMSG)) {
                debug_sub_print(DBG_RCV, "%s: msgrcv: %s\n",
                    __func__, strerror(errno));
            } else {
                // Nothing for us
            }
        } else if (bytes > 0) {
            //something to read
            if ((msg->body.status) == (CHAT_QUIT)) {
                debug_sub_print(DBG_RCV, "%s: Quit message from server " \
                    "requires processing\n", __func__);
                break;
            } else {
                debug_sub_print(DBG_RCV, "%s: Nonquit message from server " \
                    "(%d) requires processing\n", __func__, msg->body.status);
            }
        }

    }

  /* Cleanup */
  free(buf);
  return;

}


int main(int argc, char **argv) {
  char option;

  printf("RECEIVER alive: parsing options! (argc = %d\n",argc);

  while((option = getopt(argc, argv, option_string)) != -1) {
    switch(option) {
    case 'f':
      strncpy(ctrl2rcvr_fname, optarg, MAX_FILE_NAME_LEN);
      break;
    default:
      printf("invalid option %c\n",option);
      usage(argv);
      break;
    }
  }

  if(strlen(ctrl2rcvr_fname) == 0) {
    usage(argv);
  }

  printf("Receiver options ok... initializing\n");

  init_receiver();

  receive_msgs();

  return 0;
}

/*
 *   CSC469 Fall 2010 A3
 *  
 *      File:      client_main.c 
 *      Author:    Angela Demke Brown
 *      Version:   1.0.0
 *      Date:      17/11/2010
 *   
 * Please report bugs/comments to demke@cs.toronto.edu
 *
 */
#define _GNU_SOURCE /* for memrchr from string.h */

#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "client.h"
#include "defs.h"

/*************** GLOBAL VARIABLES ******************/

static char *option_string = "h:t:u:n:";

char *buf;

/* For detecting server going down */
#ifndef KA_MINUTES
    #define KA_MINUTES 2
#endif
#define KA_TIMEOUT ( 60 * KA_MINUTES )
time_t last_seen = 0;

/*
 * For TCP connection with server
 */
int tcp_sock, bytes;
#define PORT_STR_LEN 7
char server_tcp_port_str[PORT_STR_LEN];
struct addrinfo tcp_hints;
struct addrinfo *ai_result;
struct control_msghdr *cmh; /* common header pointer */

/*
 * For UDP messages to server
 */
int udp_sock;
char server_udp_port_str[PORT_STR_LEN];
struct addrinfo udp_hints;


/* For communication with chat server */
/* These variables provide some extra clues about what you will need to
 * implement.
 */
char server_host_name[MAX_HOST_NAME_LEN];

/* For control messages */
u_int16_t server_tcp_port;
struct sockaddr_in server_tcp_addr;

/* For chat messages */
u_int16_t server_udp_port;
struct sockaddr_in server_udp_addr;
int udp_socket_fd;

/* Needed for REGISTER_REQUEST */
char member_name[MAX_MEMBER_NAME_LEN];
u_int16_t client_udp_port; 

/* Initialize with value returned in REGISTER_SUCC response */
u_int16_t member_id = 0;

/* For communication with receiver process */
pid_t receiver_pid;
char ctrl2rcvr_fname[MAX_FILE_NAME_LEN];
int ctrl2rcvr_qid;

/* MAX_MSG_LEN is maximum size of a message, including header+body.
 * We define the maximum size of the msgdata field based on this.
 */
#define MAX_MSGDATA (MAX_MSG_LEN - sizeof(struct chat_msghdr))

/*
 * Send command/check common to all handle_XXX functions
 *
 * TODO: try to handle EPIPE
 */
#define TCP_SEND_BUF() \
    if ((bytes = write(tcp_sock, buf, /*ntohs*/(cmh->msg_len))) == -1) { \
        err_quit("%s: write: %s\n", __func__, strerror(errno)); \
    } else { \
        seen_server(); \
    } \
    debug_sub_print(DBG_TCP, "%s: %dB written\n", __func__, bytes);

/*
 * Receive command/check common to all handle_XXX functions
 *
 * TODO: check for ECONNREFUSED, ENOTCONN, 
 */
#define TCP_RECV_BUF() \
    if ((bytes = recv(tcp_sock, buf, MAX_MSG_LEN, 0)) == -1) { \
        err_quit("%s: recv'd: %s\n", __func__, strerror(errno)); \
    } \
    debug_sub_print(DBG_TCP, "%s: %dB recv'd\n", __func__, bytes);


#define err_quit(...) \
    fprintf(stderr, "ERROR: "); fprintf(stderr, ##__VA_ARGS__); \
    shutdown_clean(1);

/* prompt */
#define PROMPT() printf("[%s]>  ",member_name);

/************* FUNCTION DEFINITIONS ***********/

/* Updates the last_seen time for the server */
void seen_server() {
    last_seen = time(NULL);
    debug_sub_print(DBG_ACTIVE, "%s: last_seen: %d\n",
        __func__, (int) last_seen);
}

static void usage(char **argv) {

    printf("usage:\n");

#ifdef USE_LOCN_SERVER
    printf("%s -n <client member name>\n",argv[0]);
#else 
    printf("%s -h <server host name> -t <server tcp port> "
        "-u <server udp port> -n <client member name>\n",argv[0]);
#endif /* USE_LOCN_SERVER */

    exit(1);
}


/* Function to clean up after ourselves on exit, freeing any used
 * resources
 *
 * You should close any open TCP socket before calling this. We'll
 * send the quit request from here.
 */
void shutdown_clean(int ret) {

    msg_t msg;

    /* 1b. Tell receiver to quit */
    msg.mtype = RECV_TYPE;
    msg.body.status = CHAT_QUIT;
    if (msgsnd(ctrl2rcvr_qid, &msg, sizeof(struct body_s), 0) < 0) {
        err_quit("%s: msgsnd: %s\n", __func__, strerror(errno));
    }

    /* 2. Close open fd's */
    close(udp_socket_fd);

    /* 3. Wait for receiver to exit */
    waitpid(receiver_pid, 0, 0);

    /* 4. Destroy message channel */

    unlink(ctrl2rcvr_fname);
    if (msgctl(ctrl2rcvr_qid, IPC_RMID, NULL)) {
        perror("cleanup - msgctl removal failed");
    }

    free(buf);

    exit(ret);
}



int initialize_client_only_channel(int *qid)
{
    /* Create IPC message queue for communication with receiver process */

    int msg_fd;
    int msg_key;

    /* 1. Create file for message channels */

    snprintf(ctrl2rcvr_fname,MAX_FILE_NAME_LEN,"/tmp/ctrl2rcvr_channel.XXXXXX");
    msg_fd = mkstemp(ctrl2rcvr_fname);
    debug_print("ipc file: %s\n", ctrl2rcvr_fname);

    if (msg_fd  < 0) {
        perror("Could not create file for communication channel");
        return -1;
    }

    close(msg_fd);

    /* 2. Create message channel... if it already exists, delete it and try again */

    msg_key = ftok(ctrl2rcvr_fname, 42);

    if ( (*qid = msgget(msg_key, IPC_CREAT|IPC_EXCL|S_IREAD|S_IWRITE)) < 0) {
        if (errno == EEXIST) {
            if ( (*qid = msgget(msg_key, S_IREAD|S_IWRITE)) < 0) {
                perror("First try said queue existed. Second try can't get it");
                unlink(ctrl2rcvr_fname);
                return -1;
            }
            
            if (msgctl(*qid, IPC_RMID, NULL)) {
                perror("msgctl removal failed. Giving up");
                unlink(ctrl2rcvr_fname);
                return -1;
            } else {
                /* Removed... try opening again */
                if ( (*qid = msgget(msg_key, IPC_CREAT|IPC_EXCL|S_IREAD|S_IWRITE)) < 0) {
                    perror("Removed queue, but create still fails. Giving up");
                    unlink(ctrl2rcvr_fname);
                    return -1;
                }
            }
        } else {
            perror("Could not create message queue for client control <--> receiver");
            unlink(ctrl2rcvr_fname);
            return -1;
        }
    }

    return 0;
}



int create_receiver()
{
    /* Create the receiver process using fork/exec and get the port number
    * that it is receiving chat messages on.
    */

    int retries = 20;
    int numtries = 0;

    /* 1. Set up message channel for use by control and receiver processes */

    if (initialize_client_only_channel(&ctrl2rcvr_qid) < 0) {
        return -1;
    }

    /* 2. fork/exec xterm */

    receiver_pid = fork();

    if (receiver_pid < 0) {
        fprintf(stderr,"Could not fork child for receiver\n");
        return -1;
    }

    if ( receiver_pid == 0) {
        /* this is the child. Exec receiver */
        char *argv[] = {"xterm",
            "-e",
            "./receiver",
            "-f",
            ctrl2rcvr_fname,
            0
        };

        execvp("xterm", argv);
        printf("Child: exec returned. that can't be good.\n");
        exit(1);
    } 

    /* This is the parent */

    /* 3. Read message queue and find out what port client receiver is using */

    while ( numtries < retries ) {
        int result;
        msg_t msg;
        result = msgrcv(ctrl2rcvr_qid, &msg, sizeof(struct body_s), CTRL_TYPE, IPC_NOWAIT);
        if (result == -1 && errno == ENOMSG) {
            sleep(1);
            numtries++;
        } else if (result > 0) {
            if (msg.body.status == RECV_READY) {
                printf("Start of receiver successful, port %u\n",msg.body.value);
                client_udp_port = msg.body.value;
            } else {
                printf("start of receiver failed with code %u\n",msg.body.value);
                return -1;
            }
        break;
        } else {
            perror("msgrcv");
        }

    }

    if (numtries == retries) {
        /* give up.  wait for receiver to exit so we get an exit code at least */
        int exitcode;
        printf("Gave up waiting for msg.  Waiting for receiver to exit now\n");
        waitpid(receiver_pid, &exitcode, 0);
        printf("start of receiver failed, exited with code %d\n",exitcode);
    }

    return 0;
}

void open_tcp() {

    int status;

    /********************************************
     * Do TCP connection setup
     */

    /* get addr info */
    if ((status = getaddrinfo(server_host_name, server_tcp_port_str,
        &tcp_hints, &ai_result)) != 0) {
        /* TODO: try recovering to different server? */
        err_quit("%s: getaddrinfo: %s\n", __func__, gai_strerror(status));
    }

    /* get socket descriptor */
    if ((tcp_sock = socket(ai_result->ai_family, ai_result->ai_socktype,
        ai_result->ai_protocol)) == -1) {
        /* Unhandlable, fail */
        err_quit("%s: socket: %s\n", __func__, strerror(errno));
    }

    if ((connect(tcp_sock, ai_result->ai_addr, ai_result->ai_addrlen))
        == -1) {
        /* TODO: try handling ECONNREFUSED, ENETUNREACH, ETIMEDOUT */
        err_quit("%s: connect: %s\n", __func__, strerror(errno));
    }

    freeaddrinfo(ai_result);

}

void close_tcp() {
    close(tcp_sock); 
}


/*********************************************************************
 * We define one handle_XXX_req() function for each type of 
 * control message request from the chat client to the chat server.
 * These functions should return 0 on success, and a negative number 
 * on error.
 *
 * These functions all assume that buf points to a block of memory at
 * least MAX_MSG_LEN in size.
 */


int handle_register_req()
{

    /*register data area pointer */
    struct register_msgdata *rdata;

    /********************************************
     * Register with chat server
     */
    debug_sub_print(DBG_TCP, "%s: Registering with server...\n",
        __func__);

    /* Construct a REGISTER_REQUEST ************/

    /* initialize the block to all 0 */
    memset(buf, 0, MAX_MSG_LEN);

    /* message type */
    cmh->msg_type = REGISTER_REQUEST;

    /* rdata points to the data area */
    rdata = (struct register_msgdata *)cmh->msgdata;

    /* udp port: required */
    /* TODO: figure out if we have to byte-swap this or not */
    rdata->udp_port = htons(client_udp_port);

    /* member name */
    strcpy((char *)rdata->member_name, member_name);

    /* message length */
    cmh->msg_len = /*htons*/(sizeof(struct control_msghdr) +
      sizeof(struct register_msgdata) +
      strlen(member_name));

    /* Contact server, send the message */
    open_tcp();
    TCP_SEND_BUF();

    /* Catch reply */
    TCP_RECV_BUF();
    close_tcp();

    switch (cmh->msg_type) {
        case REGISTER_SUCC:
            printf("Successfully registered '%s' as member #%d\n",
                member_name, (member_id = /*ntohs*/(cmh->member_id)));
            break;
        case REGISTER_FAIL:
            printf("Registration failed: %s\n",
                (char *) (cmh->msgdata));
            /* TODO: might have failed on full server, might have failed
             * on name already in use
             */
            shutdown_clean(0);
            break;
        default:
            err_quit("%s: unexpected response to "
                "REGISTER_REQUEST: %d\n",
                __func__, cmh->msg_type);
            break;
    }
    
    return 0;
}

int handle_room_list_req()
{
    
    /* Set up request */
    memset(buf, 0, MAX_MSG_LEN);
    cmh->msg_type = ROOM_LIST_REQUEST;
    cmh->member_id = /*htons*/(member_id);
    cmh->msg_len = /*htons*/(sizeof(struct control_msghdr));

    /* Open connection, send request */
    open_tcp();
    TCP_SEND_BUF();

    /* Catch, handle response */
    TCP_RECV_BUF();
    close_tcp();

    switch (cmh->msg_type) {
        case ROOM_LIST_SUCC:
            printf("%s\n", (char *) (cmh->msgdata));
            break;
        case ROOM_LIST_FAIL:
            printf("Could not list rooms: %s\n", (char *) (cmh->msgdata)); 
            break;
        default:
            err_quit("%s: unexpected response to ROOM_LIST_REQUEST: %d\n",
                __func__, cmh->msg_type);
            break;
    }

    return 0;
}

int handle_member_list_req(char *room_name)
{

    /* Set up request */
    memset(buf, 0, MAX_MSG_LEN);
    cmh->msg_type = MEMBER_LIST_REQUEST;
    cmh->member_id = /*htons*/(member_id);
    strcpy((char *)(cmh->msgdata), room_name);
    cmh->msg_len = /*htons*/(sizeof(struct control_msghdr) +
        strlen(room_name));

    open_tcp();
    TCP_SEND_BUF();

    /* Catch, handle response */
    TCP_RECV_BUF();
    close_tcp();

    switch (cmh->msg_type) {
        case MEMBER_LIST_SUCC:
            printf("%s\n", (char *) (cmh->msgdata));
            break;
        case MEMBER_LIST_FAIL:
            printf("Could not list members in room '%s': %s\n",
                room_name, (char *) (cmh->msgdata));
            break;
        default:
            err_quit("%s: unexpected response to "
                "MEMBER_LIST_REQUEST: %d\n",
                __func__, cmh->msg_type);
            break;
    }

    return 0;
}

int handle_switch_room_req(char *room_name)
{
    /* Set up request */
    memset(buf, 0, MAX_MSG_LEN);
    cmh->msg_type = SWITCH_ROOM_REQUEST;
    cmh->member_id = /*htons*/(member_id);
    strcpy((char *)(cmh->msgdata), room_name);
    cmh->msg_len = /*htons*/(sizeof(struct control_msghdr) +
        strlen(room_name));

    open_tcp();
    TCP_SEND_BUF();

    /* Catch, handle response */
    TCP_RECV_BUF();
    close_tcp();

    switch (cmh->msg_type) {
        case SWITCH_ROOM_SUCC:
            printf("Switched to room '%s'\n", room_name);
            break;
        case SWITCH_ROOM_FAIL:
            printf("Could not switch to room '%s': %s\n",
                room_name, (char *) (cmh->msgdata));
            break;
        default:
            err_quit("%s: unexpected response to SWITCH_ROOM_REQUEST: %d\n",
                __func__, cmh->msg_type);
            break;
    }

    return 0;
}

int handle_create_room_req(char *room_name)
{
    /* Set up request */
    memset(buf, 0, MAX_MSG_LEN);
    cmh->msg_type = CREATE_ROOM_REQUEST;
    cmh->member_id = /*htons*/(member_id);
    strcpy((char *)(cmh->msgdata), room_name);
    cmh->msg_len = /*htons*/(sizeof(struct control_msghdr) +
        strlen(room_name));

    open_tcp();
    TCP_SEND_BUF();

    /* Catch, handle response */
    TCP_RECV_BUF();
    close_tcp();

    switch (cmh->msg_type) {
        case CREATE_ROOM_SUCC:
            printf("Room '%s' created.\n", room_name);
            break;
        case CREATE_ROOM_FAIL:
            printf("Could not create room '%s': %s\n",
                room_name, (char *) (cmh->msgdata));
            break;
        default:
            err_quit("%s: unexpected response to CREATE_ROOM_REQUEST: %d\n",
                __func__, cmh->msg_type);
            break;
    }

    return 0;
}


int handle_quit_req() {
    
    printf("Quitting server.\n");

    memset(buf, 0, MAX_MSG_LEN);
    cmh->msg_type = QUIT_REQUEST;
    cmh->member_id = /* htons */(member_id);
    cmh->msg_len = /* htons */(sizeof(struct control_msghdr));

    open_tcp();
    TCP_SEND_BUF();
    close_tcp();

    shutdown_clean(0); /* exits */
    return 0;
}

/* If the send of this packet is successful, TCP_SEND_BUF will
 * have updated the last_seen time. If not it currently will
 * quit.
 */
int send_keepalive() {
    /* Set up request */
    memset(buf, 0, MAX_MSG_LEN);
    cmh->msg_type = MEMBER_KEEP_ALIVE;
    cmh->member_id = /*htons*/(member_id);
    cmh->msg_len = /*htons*/(sizeof(struct control_msghdr));

    debug_sub_print(DBG_ACTIVE, "%s: sending.", __func__);

    open_tcp();
    TCP_SEND_BUF();
    close_tcp();

    return 0;
}


void create_udp_sender() {

    int ret;
    struct addrinfo *srv;
    // set up udp socket to send to chat server
    
    debug_sub_print(DBG_UDP, "%s: init UDP 'connection'...\n", __func__);
    
    memset(&udp_hints, 0, sizeof(udp_hints));
    udp_hints.ai_family = AF_INET;
    udp_hints.ai_socktype = SOCK_DGRAM;

    if ((ret = getaddrinfo(server_host_name, server_udp_port_str,
        &udp_hints, &ai_result)) != 0) {
        /* TODO: 
         * EAI_AGAIN -> try again
         * EAI_FAIL, EAI_NODATA, EAI_NONAME -> try diff server
         * EAI_SYSTEM -> print strerror, bail
         * others -> bail
         */
        err_quit("%s: getaddrinfo: %s\n",
            __func__, gai_strerror(ret));
    }

    for (srv = ai_result; srv != NULL; srv = srv ->ai_next) {

        if ((udp_sock = socket(srv->ai_family, srv->ai_socktype,
            srv->ai_protocol)) == -1) {
            /* unrecoverable, try next result */
            debug_sub_print(DBG_UDP, "%s: socket: %s\n",
                __func__, strerror(errno));
            continue;
        }
    
        if ((connect(udp_sock, srv->ai_addr, srv->ai_addrlen)) == -1) {
            /* TODO: try to handle before going to next result */
            debug_sub_print(DBG_UDP, "%s: connect: %s\n",
                __func__, strerror(errno));
            continue;
        }
    
        break;
    }

    freeaddrinfo(ai_result);
    
    if (srv == NULL) {
        err_quit("%s: failed to bind to socket\n", __func__);
    }

}


/*
 * Set up the client before accepting input.
 */
int init_client()
{
    
    /* Make room for message buffer */
    if ((buf = (char *)malloc(MAX_MSG_LEN)) == 0) {
        fprintf(stderr, "%s: malloc failed\n", __func__);
        exit(1);
    }

    /* by casting, cmh points to the beginning of the memory block, and header
    * field values can then be directly assigned */
    cmh = (struct control_msghdr *)buf;
    
    /* set up connection tcp_hints */
    memset(&tcp_hints, 0, sizeof(tcp_hints));
    tcp_hints.ai_family = AF_INET;
    tcp_hints.ai_socktype = SOCK_STREAM;

#ifdef USE_LOCN_SERVER

    /* 0. Get server host name, port numbers from location server.
     *    See retrieve_chatserver_info() in client_util.c
     */
    if((retrieve_chatserver_info(server_host_name, &server_tcp_port,
        &server_udp_port) != 0))
    {
        /* Freak out, or retry or something */
        fprintf(stderr, "%s: error contacting location server\n", __func__);
        exit(1);
    };

#endif

    /* stringify server ports for getaddrinfo */
    sprintf(server_tcp_port_str, "%d", server_tcp_port);
    sprintf(server_udp_port_str, "%d", server_udp_port);

    /********************************************
     * Initialization to allow UDP-based chat messages to chat server 
     */
    create_udp_sender();


    /********************************************
     * Spawn receiver process - see create_receiver() in this file.
     */
    debug_sub_print(DBG_RCV, "%s: init receiver process...\n",
        __func__);
    create_receiver();

    /** Send register request ******************/
    handle_register_req();

    return 0;

}



void handle_chatmsg_input(char *inputdata)
{
    /* inputdata is a pointer to the message that the user typed in.
    * This function should package it into the msgdata field of a chat_msghdr
    * struct and send the chat message to the chat server.
    */

    char *buf = (char *)malloc(MAX_MSG_LEN);
    struct chat_msghdr *chat = (struct chat_msghdr *) buf;

    if (buf == 0) {
        printf("Could not malloc memory for message buffer\n");
        shutdown_clean(1);
    }

    memset(buf, 0, MAX_MSG_LEN);
    chat->sender.member_id = /*htons*/(member_id);
    strcpy((char *)(chat->msgdata), inputdata);
    chat->msg_len = /*htons*/(sizeof(struct chat_msghdr) + strlen(inputdata));

    if ((bytes = write(udp_sock, buf, /*ntohs*/(chat->msg_len))) == -1) {
        /* TODO: handle EPIPE, bail on other errors
         */
        err_quit("%s: write: %s\n", __func__, strerror(errno));
    }
    debug_sub_print(DBG_UDP, "%s: %dB written\n", __func__, bytes);

    free(buf);
    return;
}


/* This should be called with the leading "!" stripped off the original
* input line.
* 
* You can change this function in any way you like.
*
*/
void handle_command_input(char *line)
{
    char cmd = line[0]; /* single character identifying which command */
    int len = 0;
    int goodlen = 0;
    int result;

    line++; /* skip cmd char */

    /* 1. Simple format check */

    switch(cmd) {
// TODO: change name
    case 'r':
    case 'q':
        if (strlen(line) != 0) {
            printf("Error in command format: !%c should not be followed "
                "by anything.\n",cmd);
            return;
        }
        break;

    case 'c':
    case 'm':
    case 's':
        {
            int allowed_len = MAX_ROOM_NAME_LEN;

            if (line[0] != ' ') {
                printf("Error in command format: !%c should be followed "
                    "by a space and a room name.\n",cmd);
                return;
            }
            line++; /* skip space before room name */

            len = strlen(line);
            goodlen = strcspn(line, " \t\n"); /* Any more whitespace in line? */
            if (len != goodlen) {
                printf("Error in command format: line contains extra "
                    "whitespace (space, tab or carriage return)\n");
                return;
            }
            if (len > allowed_len) {
                printf("Error in command format: name must not exceed %d "
                    "characters.\n",allowed_len);
                return;
            }
        }
        break;

    default:
        printf("Error: unrecognized command !%c\n",cmd);
        return;
        break;
    }

    /* 2. Passed format checks.  Handle the command */

    switch(cmd) {

        case 'r':
            result = handle_room_list_req();
            break;

        case 'c':
            result = handle_create_room_req(line);
            break;

        case 'm':
            result = handle_member_list_req(line);
            break;

        case 's':
            result = handle_switch_room_req(line);
            break;

        case 'q':
            result = handle_quit_req(); // does not return. Exits.
            break;

        default:
            printf("Error !%c is not a recognized command.\n",cmd);
            break;
    }

    /* Currently, we ignore the result of command handling.
    * TODO: You may want to change that.
    */

    return;
}

void main_loop() {
#define STDIN 0

    struct timeval to;
    char *newline;
    int bytes, buf_idx = 0;
    fd_set input_fds;
    char *buf = (char *)malloc(MAX_MSGDATA);
    msg_t msg;

    memset(buf, 0, MAX_MSGDATA);
    printf("\n");
    PROMPT();

    while(TRUE) {

        /* If we've not seen the server in KA_TIMEOUT seconds, send
         * a keepalive message. */
        if ((time(NULL) - last_seen) > KA_TIMEOUT) {
            send_keepalive();
        }


        /****************************************
         * Check stdin for input...
         *
         * TODO: try other values for timeout?
         */
        FD_ZERO(&input_fds);
        FD_SET(STDIN, &input_fds);
        to.tv_sec = 0;
        to.tv_usec = 0;

        select(STDIN+1, &input_fds, NULL, NULL, &to);

        if (FD_ISSET(STDIN, &input_fds)) {
            debug_sub_print(DBG_ACTIVE, "%s: stdin has input\n", __func__);
            /************************************
             * Can read from stdin, collect input in buf until
             * we've got a whole line. STDIN may be
             * line-buffered, in which case this is not
             * necessry. TODO: check/set buffering strategy
             * of stdin
             */
            bytes = read(STDIN, buf + buf_idx, MAX_MSGDATA - buf_idx);

            if (bytes <= 0) {
                err_quit("%s: reading from stdin: %s\n",
                    __func__, strerror(errno));
            } else {
                newline = (char *) (memchr(buf + buf_idx, '\n', bytes));

                if (newline) {
                    /* User pressed enter, check if control message or
                     * chat message. The functions which we pass the input
                     * on to expect null-termination.
                     */
                    *newline = '\0';

                    if (buf[0] == '!') {
                        handle_command_input(&buf[1]);
                    } else {
                        handle_chatmsg_input(buf);
                    }
                    
                    PROMPT();
                    /* done with the buffer contents, reset */
                    memset(buf, 0, MAX_MSGDATA);
                    buf_idx = 0;
                } else {
                    /* keep track of how much buffer we've used, continue
                     * waiting for a newline...
                     */
                    buf_idx += bytes;
                }
            }
        } else {
            /************************************
             * Try to read a message from the queue...
             */
            bytes = msgrcv(ctrl2rcvr_qid, &msg, sizeof(struct body_s),
                CTRL_TYPE, IPC_NOWAIT);

            if (bytes <= 0) {
                /* EAGAIN and ENOMSG are expected if there was nothing
                 * waiting for us, don't know what to do with other
                 * types of error */
                if ((errno == EAGAIN) || (errno == ENOMSG)) {
                    /* that's cool */
                    /*debug_sub_print(DBG_ACTIVE, "%s: msgrcv: %s\n",
                        __func__, strerror(errno));
                    */
                } else {
                    /* TODO: try handling this
                     */
                    err_quit("%s: msgrcv: %s\n", __func__, strerror(errno)); 
                }
            } else if (bytes > 0) {
                /* Update our timestamp if it is an activity notification from
                 * the receiver process, otherwise we're not sure what to do
                 */
                if ((msg.body.status) == SERVER_ACTIVE) {
                    seen_server();
                } else {
                    debug_sub_print(DBG_ACTIVE, "%s: Unexpected message"
                        "type from receiver (%d)\n", __func__, msg.body.status);
                }
            }
        }
    }

    free(buf);
    return;
}

int main(int argc, char **argv)
{
    char option;

    /* we want unbuffered output */
    setbuf(stdout, NULL);

    while((option = getopt(argc, argv, option_string)) != -1) {
        switch(option) {
        case 'h':
            strncpy(server_host_name, optarg, MAX_HOST_NAME_LEN);
            break;
        case 't':
            server_tcp_port = atoi(optarg);
            break;
        case 'u':
            server_udp_port = atoi(optarg);
            break;
        case 'n':
            strncpy(member_name, optarg, MAX_MEMBER_NAME_LEN);
            break;
        default:
            printf("invalid option %c\n",option);
            usage(argv);
            break;
        }
    }

#ifdef USE_LOCN_SERVER

    printf("Using location server to retrieve chatserver information\n");

    if (strlen(member_name) == 0) {
        usage(argv);
    }

#else

    if(server_tcp_port == 0 || server_udp_port == 0 ||
    strlen(server_host_name) == 0 || strlen(member_name) == 0) {
        usage(argv);
    }

#endif /* USE_LOCN_SERVER */

    init_client();

    main_loop();

    return 0;
}

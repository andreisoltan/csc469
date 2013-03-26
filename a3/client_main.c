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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <netinet/in.h>
#include <netdb.h>

#include "client.h"
#include "defs.h"
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/*************** GLOBAL VARIABLES ******************/

static char *option_string = "h:t:u:n:";

char *buf;

/*
 * For TCP connection with server
 */
int tcp_sock;
#define PORT_STR_LEN 10
char server_tcp_port_str[PORT_STR_LEN];
struct addrinfo hints;
struct addrinfo *ai_result;
struct control_msghdr *cmh; /* common header pointer */


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


/************* FUNCTION DEFINITIONS ***********/

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



void shutdown_clean() {
    /* Function to clean up after ourselves on exit, freeing any used resources */

    /* Add to this function to clean up any additional resources that you
     * might allocate.
     */

    msg_t msg;

    /* 1. Send message to receiver to quit */
    msg.mtype = RECV_TYPE;
    msg.body.status = CHAT_QUIT;
    msgsnd(ctrl2rcvr_qid, &msg, sizeof(struct body_s), 0);

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

    exit(0);
}



int initialize_client_only_channel(int *qid)
{
    /* Create IPC message queue for communication with receiver process */

    int msg_fd;
    int msg_key;

    /* 1. Create file for message channels */

    snprintf(ctrl2rcvr_fname,MAX_FILE_NAME_LEN,"/tmp/ctrl2rcvr_channel.XXXXXX");
    msg_fd = mkstemp(ctrl2rcvr_fname);

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
        &hints, &ai_result)) != 0) {
        err_quit("getaddrinfo: %s\n", gai_strerror(status));
    }

    /* get socket descriptor */
    if ((tcp_sock = socket(ai_result->ai_family, ai_result->ai_socktype,
        ai_result->ai_protocol)) == -1) {
        err_quit("tcp socket(): %s\n", strerror(errno));
    }

    if ((connect(tcp_sock, ai_result->ai_addr, ai_result->ai_addrlen))
        == -1) {
        err_quit("connect: %s\n", strerror(errno));
    }

}

void close_tcp() {
    close(tcp_sock); 
    freeaddrinfo(ai_result);
}


/*********************************************************************/

/* We define one handle_XXX_req() function for each type of 
* control message request from the chat client to the chat server.
* These functions should return 0 on success, and a negative number 
* on error.
*
* These functions all assume that buf points to a block of memory at
* least MAX_MSG_LEN in size.
*/


int handle_register_req()
{

    int bytes;

    /*register data area pointer */
    struct register_msgdata *rdata;

    open_tcp();

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

/* TODO: not this */
#define TMP_PORT 7777

    /* udp port: required */
    rdata->udp_port = htons(TMP_PORT);

    /* member name */
    strcpy((char *)rdata->member_name, member_name);

    /* message length */
    cmh->msg_len = sizeof(struct control_msghdr) +
      sizeof(struct register_msgdata) +
      strlen(member_name);

    /* send the message */
    if ((bytes = write(tcp_sock, buf, cmh->msg_len)) == -1) {
        err_quit("%s: write: %s\n", __func__, strerror(errno));
    }

    debug_sub_print(DBG_TCP, "%s: %dB written\n", __func__, bytes);

    /* wait for, receive a response */
    if ((bytes = recv(tcp_sock, buf, MAX_MSG_LEN, 0)) == -1) {
        err_quit("%s: recv'd: %s\n", __func__, strerror(errno));
    }

    debug_sub_print(DBG_TCP, "%s: %dB recv'd\n", __func__, bytes);

    switch (cmh->msg_type) {
        case REGISTER_SUCC:
            // TODO: something...
            debug_sub_print(DBG_TCP, "%s: REGISTER_SUCC as member #%d\n",
                __func__, cmh->member_id);
            member_id = cmh->member_id;
            break;
        case REGISTER_FAIL:
            // TODO: change name, reconnect?
            err_quit("%s: REGISTER_FAIL: %s\n", __func__,
                (char *) (cmh->msgdata));
            break;
        default:
            err_quit("%s: REGISTER_REQUEST returned %d\n",
                __func__, cmh->msg_type);
            break;
    }
    
    close_tcp();

    return 0;
}

int handle_room_list_req()
{

    return 0;
}

int handle_member_list_req(char *room_name)
{

    return 0;
}

int handle_switch_room_req(char *room_name)
{

    return 0;
}

int handle_create_room_req(char *room_name)
{

    return 0;
}


int handle_quit_req()
{

    return 0;
}



/*
 * Set up the client before accepting input.
 */
int init_client()
{
   
    
    /* Make room for message buffer */
    if ((buf = (char *)malloc(MAX_MSG_LEN)) == 0) {
        err_quit("%s: malloc failed\n", __func__);
    }

    /* by casting, cmh points to the beginning of the memory block, and header
    * field values can then be directly assigned */
    cmh = (struct control_msghdr *)buf;
    
    /* set up connection hints */
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    /* stringify server port for getaddrinfo */
    sprintf(server_tcp_port_str, "%d", server_tcp_port);

#ifdef USE_LOCN_SERVER

    /* 0. Get server host name, port numbers from location server.
    *    See retrieve_chatserver_info() in client_util.c
    */

#endif

    /********************************************
     * Initialization to allow UDP-based chat messages to chat server 
     */
    debug_sub_print(DBG_UDP, "%s: Should init UDP here...\n", __func__);


    /********************************************
     * Spawn receiver process - see create_receiver() in this file.
     */
    debug_sub_print(DBG_RCV, "%s: Should init receiver process here...\n",
        __func__);

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

    if (buf == 0) {
        printf("Could not malloc memory for message buffer\n");
        shutdown_clean();
        exit(1);
    }

    bzero(buf, MAX_MSG_LEN);


    /**** YOUR CODE HERE ****/


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

void get_user_input()
{
    char *buf = (char *)malloc(MAX_MSGDATA);
    char *result_str;

    while(TRUE) {

        bzero(buf, MAX_MSGDATA);

        printf("\n[%s]>  ",member_name);

        result_str = fgets(buf,MAX_MSGDATA,stdin);

        if (result_str == NULL) {
            printf("Error or EOF while reading user input.  Guess we're done.\n");
            break;
        }

        /* Check if control message or chat message */

        if (buf[0] == '!') {
            /* buf probably ends with newline.  If so, get rid of it. */
            int len = strlen(buf);
            if (buf[len-1] == '\n') {
                buf[len-1] = '\0';
            }
            handle_command_input(&buf[1]);
        } else {
            handle_chatmsg_input(buf);
        }
    }

    free(buf);

}


int main(int argc, char **argv)
{
    char option;

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

    get_user_input();

    return 0;
}
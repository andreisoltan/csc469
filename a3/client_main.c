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

/* Common send/receive structures for incoming/outgoing packets
 * (shared because we're single-threaded) */
char *buf;
struct control_msghdr *cmh;

/* For detecting server going down */
time_t last_seen = 0;

/* For TCP control message connection with server */
#define PORT_STR_LEN 7 /* max length of a port number string */
u_int16_t server_tcp_port;
char server_tcp_port_str[PORT_STR_LEN];
int tcp_sock, bytes;
struct addrinfo tcp_hints;
struct addrinfo *ai_result;
struct sockaddr_in server_tcp_addr;

/* For UDP chat messages to server */
u_int16_t server_udp_port;
char server_udp_port_str[PORT_STR_LEN];
int udp_socket_fd;
struct addrinfo udp_hints;
struct sockaddr_in server_udp_addr;

/* CLIENT STATE ********************************/
char server_host_name[MAX_HOST_NAME_LEN];
char server_room_name[MAX_ROOM_NAME_LEN+1];
char member_name[MAX_MEMBER_NAME_LEN+1];
u_int16_t client_udp_port; 
u_int16_t member_id = 0;

/* For communication with receiver process *****/
pid_t receiver_pid;
char ctrl2rcvr_fname[MAX_FILE_NAME_LEN];
int ctrl2rcvr_qid;

/* MAX_MSG_LEN is maximum size of a message, including header+body.
 * We define the maximum size of the msgdata field based on this.
 */
#define MAX_MSGDATA (MAX_MSG_LEN - sizeof(struct chat_msghdr))

/* prompt */
#define PROMPT() printf("[%s]>  ",member_name);

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


/* Function to clean up after ourselves on exit, freeing any used
 * resources.
 */
void shutdown_clean(int ret) {

    msg_t msg;

    /* 1b. Tell receiver to quit */
    msg.mtype = RECV_TYPE;
    msg.body.status = CHAT_QUIT;
    if (msgsnd(ctrl2rcvr_qid, &msg, sizeof(struct body_s), 0) < 0) {
        fprintf(stderr, "%s: msgsnd: %s\n", __func__, strerror(errno));
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

/* Return 0 on success or some errno on failure */
int open_tcp() {

    int status, ret = 0;

    /********************************************
     * Do TCP connection setup
     */

    /* get addr info */
    if ((status = getaddrinfo(server_host_name, server_tcp_port_str,
        &tcp_hints, &ai_result)) != 0) {
        ret = errno;
        if (status == EAI_SYSTEM) {
            fprintf(stderr, "%s: getaddrinfo: %s\n",
                __func__, strerror(ret));
        } else {
            fprintf(stderr, "%s: getaddrinfo: %s\n",
                __func__, gai_strerror(status));
            ret = status;
        }
        return ret;
    }

    /* get socket descriptor */
    if ((tcp_sock = socket(ai_result->ai_family, ai_result->ai_socktype,
        ai_result->ai_protocol)) == -1) {
        ret = errno;
        fprintf(stderr, "%s: socket: %s\n", __func__, strerror(errno));
        return ret;
    }

    if ((connect(tcp_sock, ai_result->ai_addr, ai_result->ai_addrlen))
        == -1) {
        ret = errno;
        fprintf(stderr, "%s: connect: %s\n", __func__, strerror(errno));
        return ret;
    }

    freeaddrinfo(ai_result);
    return ret;
}

void close_tcp() {
    close(tcp_sock); 
}

/* Updates the last_seen time for the server */
void seen_server() {
    last_seen = time(NULL);
    debug_sub_print(DBG_ACTIVE, "%s: last_seen: %d\n",
        __func__, (int) last_seen);
}

/* Updates our copy of the current room name */
void update_room_state(char *new_room) {
    strncpy(server_room_name, new_room, MAX_ROOM_NAME_LEN);
    /* make sure our string is still terminated */
    server_room_name[MAX_ROOM_NAME_LEN] = '\0';
}

/* Prepend an underscore to the current username -- used in the case that
 * we're reconnecting to a server after a failure and our name is already
 * in use. */
void bump_username() {
    int i;

    debug_sub_print(DBG_FAULT, "Changing name from '%s' to ", member_name);

    /* copy each character at i to i+1 */
    for (i = MAX_MEMBER_NAME_LEN; i > 0; i--) {
        member_name[i] = member_name[i-1];
    }

    member_name[0] = '_';
    member_name[MAX_MEMBER_NAME_LEN] = '\0';

    debug_sub_print(DBG_FAULT, "'%s'\n", member_name);
}

/******************************************************************************
 * COMMAND HANDLERS ***********************************************************
 *****************************************************************************/

/* Implements the general case for command message send-receive
 *
 * If any of our underlying calls return an error, we will pass
 * that up to our caller immediately.
 *
 * Otherwise we'll examine the response.
 *
 * Returns:
 * - some errno from one of our calls
 * - INVALID_ID if the server does not recognize our member id
 * - COMMAND_SUCC on success
 * - COMMAND_FAIL on failure
 * - BOGUS_RESPONSE on an unexpected reply
 *
 * Takes advantage of the fact that for a request message type
 * x, the corresponding success and fail responses are x+1
 * and x+2 respectively. We encode that relationship in the macros
 * CODE_SUCC() and CODE_FAIL(). Keepalive and quit messages do not
 * wait for a response.
 */
int generic_command(int msg_type, char *arg, char *buf) {

    int ret = 0, bytes = 0;
    struct control_msghdr *cmh = (struct control_msghdr *) buf;

    memset(buf, 0, MAX_MSG_LEN);
    cmh->msg_type = msg_type;
    cmh->member_id = /*htons*/(member_id);
    cmh->msg_len = /*htons*/(sizeof(struct control_msghdr));

    if (arg) {
        strcpy((char *)(cmh->msgdata), arg);
        cmh->msg_len += strlen(arg);
    }
    
    if (msg_type == REGISTER_REQUEST) {
        struct register_msgdata *rdata;
        rdata = (struct register_msgdata *)cmh->msgdata;
        /* send registration data */
        rdata->udp_port = htons(client_udp_port);
        strcpy((char *)rdata->member_name, member_name);
        cmh->msg_len +=
            sizeof(struct register_msgdata) + strlen(member_name);
    }

    /* OPEN */
    if ((ret = open_tcp()) != 0) {
        return ret;
    }   

    /* WRITE */
    if ((bytes = write(tcp_sock, buf, /*ntohs*/(cmh->msg_len))) == -1) {
        ret = errno;
        fprintf(stderr, "%s: write: %s\n", __func__, strerror(errno));
        return ret;
    } else {
        seen_server();
    }   

    debug_sub_print(DBG_TCP, "%s: %dB written\n", __func__, bytes);

    /* Response expected? */
    if (msg_type < MEMBER_KEEP_ALIVE) {
        if ((bytes = recv(tcp_sock, buf, MAX_MSG_LEN, 0)) == -1) {
            ret = errno;
            fprintf(stderr, "%s: recv'd: %s\n", __func__, strerror(errno));
            return ret;
        }
        close_tcp();
        debug_sub_print(DBG_TCP, "%s: %dB recv'd\n", __func__, bytes);

        if ((cmh->msg_type) == CODE_SUCC(msg_type)) {
            return COMMAND_SUCC;
        } else if ((cmh->msg_type) == CODE_FAIL(msg_type)) {
            if ((strstr((char *) (cmh->msgdata), "Member id invalid!"))
                == (char *) (cmh->msgdata))
            {
                return ID_INVALID;
            }
            return COMMAND_FAIL;
        } else {
            fprintf(stderr, "Unexpected response (%d)\n", cmh->msg_type);
            return BOGUS_RESPONSE;
        }
    } else {
        close_tcp();
    }
    return ret;
}

/*********************************************************************
 * We define one handle_XXX_req() function for each type of 
 * control message request from the chat client to the chat server.
 * These functions should return 0 on success.
 *
 * These functions all assume that buf points to a block of memory at
 * least MAX_MSG_LEN in size.
 */

int handle_register_req()
{
    int ret = generic_command(REGISTER_REQUEST, NULL, buf);
    debug_sub_print(DBG_TCP, "%s: Registering with server...\n",
        __func__);

    switch (ret) {
        case COMMAND_SUCC:
            printf("Successfully registered '%s' as member #%d\n",
                member_name, (member_id = /*ntohs*/(cmh->member_id)));
            break;
        case COMMAND_FAIL:
            fprintf(stderr, "Registration failed: %s\n",
                (char *) (cmh->msgdata));
            /* Determine cause of failure with a great big smelly
             * HAAAAAAAAAAAAAAAAAAAAAAAAAAACK. A nice protocol would
             * allow us to distinguish these cases without string-
             * compares */
            if ((strstr((char *) (cmh->msgdata), "Name"))
                == (char *) (cmh->msgdata))
            {
                /* If we find "Name" at the beginning of the message
                 * our name is in use. */
                ret = NAME_IN_USE;
            } else {
                /* Only other case for rejection (based on examination
                 * of the server code) is if the server is full. */
                ret = SERVER_FULL;
            }
        default:
            return ret;
    }
    return ret;
}

int handle_room_list_req() {
    int ret = generic_command(ROOM_LIST_REQUEST, NULL, buf);
    switch(ret) {
        case COMMAND_SUCC:
            printf("%s\n", (char *) (cmh->msgdata));
            break;
        case COMMAND_FAIL:
            printf("Could not list rooms: %s\n", (char *) (cmh->msgdata));
            break;
        default:
            return ret;
    }
    return 0;
}

int handle_member_list_req(char *room_name) {
    int ret = generic_command(MEMBER_LIST_REQUEST, room_name, buf);
    switch (ret) {
        case COMMAND_SUCC:
            printf("%s\n", (char *) (cmh->msgdata));
            break;
        case COMMAND_FAIL:
            printf("Could not list members in room '%s': %s\n",
                room_name, (char *) (cmh->msgdata));
            break;
        default:
            return ret;
    }
    return 0;
}

int handle_switch_room_req(char *room_name)
{
    int ret = generic_command(SWITCH_ROOM_REQUEST, room_name, buf);
    switch (ret) {
        case COMMAND_SUCC:
            printf("Switched to room '%s'\n", room_name);
            update_room_state(room_name);
            break;
        case COMMAND_FAIL:
            printf("Could not switch to room '%s': %s\n",
                room_name, (char *) (cmh->msgdata));

            /* Classify the failure... */
            if ((strstr((char *) (cmh->msgdata), "No room avail"))
                == (char *) (cmh->msgdata))
            {   /* "No room available yet!" */
                return ZERO_ROOMS;
            } else if ((strstr((char *) (cmh->msgdata), "Room not found"))
                == (char *) (cmh->msgdata))
            {   /* "Room not found!" */
                return ROOM_NOT_FOUND;
            } else if ((strstr((char *) (cmh->msgdata), "Room is full"))
                == (char *) (cmh->msgdata))
            {   /* "Room is full!" */
                return ROOM_FULL;
            }
            break;
        default:
            return ret;
    }
    return 0;
}

int handle_create_room_req(char *room_name)
{
    int ret = generic_command(CREATE_ROOM_REQUEST, room_name, buf);
    switch (ret) {
        case COMMAND_SUCC:
            printf("Room '%s' created.\n", room_name);
            break;
        case COMMAND_FAIL:
            printf("Could not create room '%s': %s\n",
                room_name, (char *) (cmh->msgdata));

            /* Classify the failure... */
            if ((strstr((char *) (cmh->msgdata), "Number of rooms"))
                == (char *) (cmh->msgdata))
            {   /* "Number of rooms reached maximum!" */
                ret = MAX_ROOMS;
            } else if ((strstr((char *) (cmh->msgdata), "Room exists"))
                == (char *) (cmh->msgdata))
            {   /* "Room exists!" */
                ret = ROOM_EXISTS;
            } else { /* Only other possibility based on server code */
                ret = ROOM_NAME_TOOOO_LOOOONG;
            }
            break;
        default:
            return ret;
    }
    return 0;
}


int handle_quit_req() {
    int ret = 0; 
    printf("Quitting server.\n");
    ret = generic_command(QUIT_REQUEST, NULL, buf);
    shutdown_clean(0); /* exits */
    return ret;
}

/* If the send of this packet is successful, generic_command will
 * have updated the last_seen time. */
int handle_keepalive() {
    int ret = generic_command(MEMBER_KEEP_ALIVE, NULL, buf);
    debug_sub_print(DBG_ACTIVE, "Ah, ah, ah, ah, staying alive\n");
    return ret;
}

/* Decides whether or not to send a keepalive packet */
int check_keepalive() {
    int retries = RETRY_COUNT,
        pause = RETRY_PAUSE,
        result = 0;

    if ((time(NULL) - last_seen) > KA_TIMEOUT)
        result = retry_handler(handle_keepalive, NULL, &retries, &pause);

    return result;
}

/* end COMMAND HANDLERS ******************************************************/

/* Returns 0 on success, an errno value on failure */
int create_udp_sender() {

    int status, ret = 0;
    struct addrinfo *srv;
    // set up udp socket to send to chat server
    
    debug_sub_print(DBG_UDP, "%s: init UDP 'connection'...\n", __func__);
    
    memset(&udp_hints, 0, sizeof(udp_hints));
    udp_hints.ai_family = AF_INET;
    udp_hints.ai_socktype = SOCK_DGRAM;

    if ((status = getaddrinfo(server_host_name, server_udp_port_str,
        &udp_hints, &ai_result)) != 0) {
        ret = errno;
        if (status == EAI_SYSTEM) {
            fprintf(stderr, "%s: getaddrinfo: %s\n",
                __func__, strerror(ret));
        } else {
            fprintf(stderr, "%s: getaddrinfo: %s\n",
                __func__, gai_strerror(status));
            ret = status;
        }
        return ret;
    }

    for (srv = ai_result; srv != NULL; srv = srv ->ai_next) {

        if ((udp_socket_fd = socket(srv->ai_family, srv->ai_socktype,
            srv->ai_protocol)) == -1) {
            /* unrecoverable, try next result */
            ret = errno;
            debug_sub_print(DBG_UDP, "%s: socket: %s\n",
                __func__, strerror(errno));
            continue;
        }
    
        if ((connect(udp_socket_fd, srv->ai_addr, srv->ai_addrlen)) == -1) {
            ret = errno;
            /* TODO: try to handle before going to next result */
            debug_sub_print(DBG_UDP, "%s: connect: %s\n",
                __func__, strerror(errno));
            continue;
        }
    
        break;
    }

    freeaddrinfo(ai_result);
    
    if (srv == NULL) {
        fprintf(stderr, "%s: failed to bind to socket\n", __func__);
    } else {
        ret = 0;
    }

    return ret;

}

/* Attempts to rejoin the room we were in prior to being disconnected
 * ASSUMES THAT WE ARE ALREADY RECONNECTED AND THAT THE ROOM NAME
 * IS IN server_room_name
 *
 * create is treated as a boolean -- to attempt creation of the room
 * or not
 * 
 * Since we're making a best effort, we'll only pass along failure
 * codes for errors we've not considered here. If there was no room for
 * us to create our room for instance, that is ok -- if there is a
 * network failure our caller should know */
int rejoin(char create) {
    int retries = RETRY_COUNT,
        pause = RETRY_PAUSE,
        ret = 0;

    /* Try switching to the room we were in */
    ret = retry_handler(handle_switch_room_req,
        server_room_name, &retries, &pause);

    switch (ret) {
        case COMMAND_SUCC:
            /* cool, done */
            return ret;
        case ZERO_ROOMS:
        case ROOM_NOT_FOUND:
            if (create) {
                /* try to recreate, then join */
                printf("Attempting to recreate room '%s'...\n",
                    server_room_name);
                ret = retry_handler(handle_create_room_req, server_room_name,
                    &retries, &pause);

                switch (ret) {
                    case COMMAND_SUCC:
                    case ROOM_EXISTS:
                        /* cool, retry join */
                        return rejoin(FALSE);
                    case MAX_ROOMS:
                    case ROOM_NAME_TOOOO_LOOOONG:
                        /* can't, oh well */
                        ret = 0;
                        break;
                    default:
                        fprintf(stderr, "Could not recreate room '%s'.\n",
                            server_room_name);
                        break;
                }
            }
            break;
        case ROOM_FULL:
            /* can't join */
            ret = 0;
            break;
        default:
            /* some other problem */
            fprintf(stderr, "Could not rejoin room '%s'.\n",
                server_room_name);
            break;
    }

    return ret;
}


/*
 * Set up the client before accepting input.
 */
int init_client()
{
    int retries = RETRY_COUNT,
        pause = RETRY_PAUSE, 
        ret = 0;

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
        shutdown_clean(1);
    };

#endif

    /* stringify server ports for getaddrinfo */
    sprintf(server_tcp_port_str, "%d", server_tcp_port);
    sprintf(server_udp_port_str, "%d", server_udp_port);

    /********************************************
     * Initialization to allow UDP-based chat messages to chat server 
     */
    if ((ret = create_udp_sender()) != 0) {
        fprintf(stderr, "Could not create UDP socket\n");
        return ret;
    }
    
    /** Send register request ******************/
    while ((ret =
        retry_handler(handle_register_req, NULL, &retries, &pause))
        != 0)
    {
        switch (ret) {
            case NAME_IN_USE:
                // bump name, try again
                bump_username();
                break;
            case SERVER_FULL:
                shutdown_clean(0);
                break;
            default:
#ifdef USE_LOCN_SERVER
                /* Some communication error, try refreshing parameters
                 * from the locn server if we're using it */
                if((retrieve_chatserver_info(server_host_name,
                    &server_tcp_port, &server_udp_port) != 0))
                {
#endif 
                    // unrecoverable, die
                    shutdown_clean(1);
                    break;
#ifdef USE_LOCN_SERVER
                }
#endif
        }
    }

    /* If we successfully registered, see if we've got a room
     * name recorded -- if so we are reconnecting and we should
     * try to get back into that same room */
    if ((ret == 0) && (server_room_name[0] != 0)) {
        printf("Attempting to rejoin room '%s'...\n", server_room_name);
        ret = rejoin(TRUE);
    }

    return ret;
}

/* Return 0 on success */
int handle_chatmsg_input(char *inputdata)
{
    /* inputdata is a pointer to the message that the user typed in.
    * This function should package it into the msgdata field of a chat_msghdr
    * struct and send the chat message to the chat server.
    */
    int ret = 0, tries = 3;
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

    while ( (tries-- > 0) &&
        ((write(udp_socket_fd, buf, /*ntohs*/(chat->msg_len)))
        != (chat->msg_len)) )
    {
        ret = errno;
        printf("%s: retrying...\n", __func__);
        sleep(RETRY_PAUSE);
    }

    if (tries < 0) {
        return (ret);
    }
    debug_sub_print(DBG_UDP, "%s: %dB written\n", __func__, bytes);


    free(buf);
    return ret;
}


/* This should be called with the leading "!" stripped off the original
* input line.
* 
* You can change this function in any way you like.
*
*/
int handle_command_input(char *line)
{
    char cmd = line[0]; /* single character identifying which command */
    int len = 0;
    int goodlen = 0;
    int result = 0;
    int retries = RETRY_COUNT,
        pause = RETRY_PAUSE;

    line++; /* skip cmd char */

    /* 1. Simple format check */

    switch(cmd) {
// TODO: change name
    case 'r':
    case 'q':
        if (strlen(line) != 0) {
            printf("Error in command format: !%c should not be followed "
                "by anything.\n",cmd);
            return 0;
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
                return 0;
            }
            line++; /* skip space before room name */

            len = strlen(line);
            goodlen = strcspn(line, " \t\n"); /* Any more whitespace in line? */
            if (len != goodlen) {
                printf("Error in command format: line contains extra "
                    "whitespace (space, tab or carriage return)\n");
                return 0;
            }
            if (len > allowed_len) {
                printf("Error in command format: name must not exceed %d "
                    "characters.\n",allowed_len);
                return 0;
            }
        }
        break;

    default:
        printf("Error: unrecognized command !%c\n",cmd);
        return 0;
        break;
    }

    /* 2. Passed format checks.  Handle the command */
    do {
        switch(cmd) {
            case 'r':
                result = retry_handler(handle_room_list_req, NULL,
                    &retries, &pause);
                break;
            case 'c':
                result = retry_handler(handle_create_room_req, line,
                    &retries, &pause);
                break;

            case 'm':
                result = retry_handler(handle_member_list_req, line,
                    &retries, &pause);
                break;

            case 's':
                result = retry_handler(handle_switch_room_req, line,
                    &retries, &pause);
                break;

            case 'q':
                /* exits if successfull */
                result = retry_handler(handle_quit_req, NULL,
                    &retries, &pause);
                break;

            default:
                printf("Error !%c is not a recognized command.\n",cmd);
                break;
        }

        switch (result) {
            /* Success, or errors that won't be helped by a resend
             * or reconnection */
            case COMMAND_SUCC:
            case ROOM_NOT_FOUND:
            case MAX_ROOMS:
            case ROOM_EXISTS:
            case ROOM_NAME_TOOOO_LOOOONG:
            case ROOM_FULL:
            case ZERO_ROOMS:
                return 0;
            case_RETURN /* We should return up to main and reconnect */
                return result;
/*
            case_RETRYABLE
                retries--;
                fprintf(stderr, "%s: error, retrying...\n", __func__);
                sleep(pause);
                break;
*/
            default:
                fprintf(stderr, "%s: unexpected error (%d)\n", __func__);
                return result;
                break;
        }
         
    } while((result != 0) && (retries > 0));

    /* Error if we ran out of retries */
    return (retries <= 0)? result : 0;
}

int main_loop() {
#define STDIN 0

    int ret = 0;
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
        if ((ret = check_keepalive()) != 0) {
            fprintf(stderr, "Problem sending keepalive packet.\n");
            return ret;
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
                fprintf(stderr, "%s: reading from stdin: %s\n",
                    __func__, strerror(errno));
                shutdown_clean(1);
            } else {
                newline = (char *) (memchr(buf + buf_idx, '\n', bytes));

                if (newline) {
                    /* User pressed enter, check if control message or
                     * chat message. The functions which we pass the input
                     * on to expect null-termination.
                     */
                    *newline = '\0';

                    if (buf[0] == '!') {
                        ret = handle_command_input(&buf[1]);
                    } else {
                        ret = handle_chatmsg_input(buf);
                    }

                    if (ret) {
                        fprintf(stderr, "Error communicating with server\n");
                        return ret;
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
                    fprintf(stderr, "%s: msgrcv: unexpected error: %s\n",
                        __func__, strerror(errno));
                    shutdown_clean(1);
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

    debug_print("%s: Hm, shouldn't be here.\n", __func__);
    return -1;
}

int main(int argc, char **argv)
{
    int ret = -1, tries = RETRY_COUNT;
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

    /********************************************
     * Spawn receiver process - see create_receiver() in this file.
     */
    debug_sub_print(DBG_RCV, "%s: init receiver process...\n", __func__);

    if ((create_receiver() == -1)) {
        fprintf(stderr, "Could not create receiver process\n");
        exit(1);
    }

    /********************************************
     * Initialize client and start accepting input. 
     */
    server_room_name[0] = '\0'; 
    while (ret != 0) {
       
        tries = RETRY_COUNT; 
        while ((ret = init_client()) != 0) {
            if (tries-- <= 0) {
                /* If we fail to connect RETRY_COUNT times,
                 * just give up.
                 */
                fprintf(stderr, "Problem initiating connection with "
                    "server.\n");
                shutdown_clean(ret);
            }
        }

        if ((ret = main_loop()) != 0) {
            fprintf(stderr, "Trying to reconnect...\n");
        }
    }

    shutdown_clean(ret);
    return 0;
}

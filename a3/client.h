/*
 *   CSC469 Fall 2010 A3
 *  
 *      File:      client.h 
 *      Author:    Angela Demke Brown
 *      Version:   1.0.0
 *      Date:      17/11/2010
 *   
 * Please report bugs/comments to demke@cs.toronto.edu
 *
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <sys/wait.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "defs.h"

/* Map command type, c, to response types */
#define CODE_SUCC(c) ((c)+1)
#define CODE_FAIL(c) ((c)+2)

/* debug junk *******************************************************/

/* Flags for debug printing. Define more as needed. */
#ifndef DEBUG
    #define DEBUG 0
#endif
#ifndef DBG_LOC
    #define DBG_LOC 0
#endif
#ifndef DBG_FAULT
    #define DBG_FAULT 0
#endif
#ifndef DBG_ACTIVE
    #define DBG_ACTIVE 0
#endif
#ifndef DBG_TCP
    #define DBG_TCP 0
#endif
#ifndef DBG_UDP
    #define DBG_UDP 0
#endif
#ifndef DBG_RCV
    #define DBG_RCV 0
#endif

/*
 * The debug_print macro is borrowed from Jonathan Leffler, here:
 *   http://stackoverflow.com/questions/1644868/c-define-macro-for-debug-printing
 *
 * Besides the DEBUG macro, this one check an additional user supplied
 * conditional -- for instance a subsystem specific flag, e.g.:
 *   debug_sub_print(DBG_TCP, "oh, wow, TCP happened!\n");
 */
#define debug_sub_print(sub_sys, ...) \
    do { if ((DEBUG) && (sub_sys)) \
        fprintf(stderr, ##__VA_ARGS__); } while (0)

/* Plain debug_print -- only checks for definition of DEBUG */
#define debug_print(...) debug_sub_print(DEBUG, ##__VA_ARGS__) ;

/* END debug junk ***************************************************/

/*** Defines for client control <--> receiver communication ***/

struct body_s {
  u_int16_t status;
  u_int16_t value;
} ;

/* Compiler was complaining about this "typedef struct msgbuf" being a \
 * redefinition of one found in /usr/include/sys/msg.h. We only refer to
 * it by the type name in our work here.
 */
typedef struct our_msgbuf {
  long mtype;
  struct body_s body;
} msg_t;

/* For keepalive */
#ifndef KA_MINUTES
    #define KA_MINUTES 2
#endif
#define KA_TIMEOUT ( 60 * KA_MINUTES )

#define CTRL_TYPE 1 /* mtype for messages to control process */
#define RECV_TYPE 2 /* mtype for messages to receiver process */

/* Not many options for status right now. You may add more if you wish. */

/* receiver can tell controller it's ready and supply port number,
 * or not ready and a failure code.  Controller can tell receiver to quit.
 * Receiver can tell the controller that it has just seen some activity
 * from the chat server.
 */

#define RECV_READY    1
#define RECV_NOTREADY 2
#define CHAT_QUIT     3
#define SERVER_ACTIVE 4

/* Failure codes from receiver. */
#define NO_SERVER     10
#define SOCKET_FAILED 11
#define BIND_FAILED   12
#define NAME_FAILED   13

/* Failure codes for client -- we use high numbers to avoid
 * conflicting with errno values. */
#define SERVER_FULL     501
#define NAME_IN_USE     502
#define BOGUS_RESPONSE  503
#define SERVER_DOWN     504
#define RETRY_SERVER    505
#define REG_FAILED      506
#define ID_INVALID      507
#define COMMAND_FAIL    508
#define COMMAND_SUCC    0
#define MAX_ROOMS       509
#define ROOM_EXISTS     510
#define ROOM_NOT_FOUND  511
#define ROOM_NAME_TOOOO_LOOOONG 512
#define ROOM_FULL       513
#define ZERO_ROOMS      514

/* Fault handling */
#define RETRY_PAUSE    1 /* base pause between retries, we back this off */
#define RETRY_COUNT    3 /* retry attempts */

/* Connection errors we'd like to retry on */
#define case_RETRYABLE \
            case EAI_AGAIN:     /* temporary getaddrinfo failure */ \
            case EAGAIN:        /* maybe a transient failure */ \
            case EPROTO:        /* locn server response in unexpected format */ \
            case EHOSTUNREACH:  /* maybe a transient failure */ \
            case ENETDOWN:      /* maybe a transient failure */ \
            case ECOMM:         /* Communication error on send */ \
            case ENETRESET:     /* maybe a transient failure */ \
            case ENETUNREACH:   /* maybe a transient failure */

extern char *optarg; /* For option parsing */
extern int retry_handler(int (*handler)(char*), char *arg, int *retries, int *pause);
extern int retrieve_chatserver_info(char *chatserver_name, u_int16_t *tcp_port, u_int16_t *udp_port);

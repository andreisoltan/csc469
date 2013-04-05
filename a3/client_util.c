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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <netinet/in.h>
#include <netdb.h>

#include "client.h"

static void build_req(char *buf)
{
  /* Write an HTTP GET request for the chatserver.txt file into buf */

  int nextpos = 0;

  sprintf(&buf[nextpos],"GET /~csc469h/winter/chatserver.txt HTTP/1.0\r\n");
  nextpos = strlen(buf);

  sprintf(&buf[nextpos],"\r\n");
}

static char *skip_http_headers(char *buf)
{
  /* Given a pointer to a buffer which contains an HTTP reply,
   * skip lines until we find a blank, and then return a pointer
   * to the start of the next line, which is the reply body.
   * 
   * DO NOT call this function if buf does not contain an HTTP
   * reply message.  The termination condition on the while loop 
   * is ill-defined for arbitrary character arrays, and may lead 
   * to bad things(TM). 
   *
   * Feel free to improve on this.
   */

  char *curpos;
  int n;
  char line[256];

  curpos = buf;

  while ( sscanf(curpos,"%256[^\n]%n",line,&n) > 0) {
    if (strlen(line) == 1) { /* Just the \r was consumed */
      /* Found our blank */
      curpos += n+1; /* skip line and \n at end */
      break;
    }
    curpos += n+1;
  }

  return curpos;
}


int _retrieve_chatserver_info(char *chatserver_name,
    u_int16_t *tcp_port, u_int16_t *udp_port)
{
    int locn_socket_fd;
    char *buf, *body;
    char location_fmt[11];
    int buflen;
    int code;
    int  n;
    int ret = 0;
    int status;

    struct addrinfo tcp_hints;
    struct addrinfo *ai_result;

    /* Initialize locnserver_addr. 
     * We use a text file at a web server for location info
     * so this is just contacting the CDF web server */

    /* 1. Set up TCP connection to web server "www.cdf.toronto.edu", port 80 */
    memset(&tcp_hints, 0, sizeof(struct addrinfo));
    tcp_hints.ai_family = AF_INET;
    tcp_hints.ai_socktype = SOCK_STREAM;
    
    debug_sub_print(DBG_LOC, "%s: getaddrinfo\n", __func__);
    if ((status = getaddrinfo("www.cdf.toronto.edu", "80",
        &tcp_hints, &ai_result)) != 0)
    {
        ret = errno;
        fprintf(stderr, "%s: getaddrinfo: %s\n", __func__,
            gai_strerror(status));
        return ret;
    }

    debug_sub_print(DBG_LOC, "%s: socket()\n", __func__);
    /* get socket descriptor */
    if ((locn_socket_fd = socket(ai_result->ai_family, ai_result->ai_socktype,
        ai_result->ai_protocol)) == -1)
    {
        ret = errno;
        /* Unhandlable, fail */
        fprintf(stderr, "%s: socket: %s\n", __func__, strerror(errno));
        return ret;
    }

    debug_sub_print(DBG_LOC, "%s: connecting\n", __func__);
    if ((connect(locn_socket_fd, ai_result->ai_addr, ai_result->ai_addrlen))
        == -1)
    {
        ret = errno;
        fprintf(stderr, "%s: connect: %s\n", __func__, strerror(errno));
        return ret;
    }

    debug_sub_print(DBG_LOC, "%s: freeing addrinfo\n", __func__);
    freeaddrinfo(ai_result);

    debug_sub_print(DBG_LOC, "%s: writing request\n", __func__);
    /* 2. write HTTP GET request to socket */
    if ((buf = (char *)malloc(MAX_MSG_LEN)) == NULL)
    {
        fprintf(stderr, "%s: malloc failed\n", __func__);
        return ENOMEM;
    }

    bzero(buf, MAX_MSG_LEN);
    build_req(buf);
    buflen = strlen(buf);

    if ((write(locn_socket_fd, buf, buflen) <= 0)) 
    {
        ret = errno;
        close(locn_socket_fd);
        fprintf(stderr, "%s: write failed %s\n",
            __func__, strerror(errno));
        return ret;
    }

    debug_sub_print(DBG_LOC, "%s: reading reply\n", __func__);
    /* 3. Read reply from web server */
    if ((read(locn_socket_fd, buf, MAX_MSG_LEN) <= 0)) 
    {
        ret = errno;
        close(locn_socket_fd);
        fprintf(stderr, "%s: read failed %s\n",
            __func__, strerror(errno));
        return ret;
    }
    
    debug_sub_print(DBG_LOC, "%s: closing socket\n", __func__);
    close(locn_socket_fd);
    
    /* 4. Check if request succeeded.  If so, skip headers and initialize
     *    server parameters with body of message.  If not, print the 
     *    STATUS-CODE and STATUS-TEXT and return -1.
     */

    /* Ignore version, read STATUS-CODE into variable 'code' , and record
     * the number of characters scanned from buf into variable 'n'
     */
    debug_sub_print(DBG_LOC, "%s: scanning\n", __func__);
    sscanf(buf, "%*s %d%n", &code, &n);
    
    /* Check code, if not 2xx, bail.
     * (Later we should try to handle redirects, retrying)
     */
    if ((n < 3) || (code < 200) || (code >= 300)) {
        fprintf(stderr, "%s: code=%d n=%d\n", __func__, code, n);
        return ENOMSG;
    }

    /* Use skip_http_headers to jump to body. */
    body = skip_http_headers(buf);
    
    /* Scan hostname, ports from body (sanity checks?) */
    sprintf(location_fmt, "%%%ds %%u %%u", MAX_HOST_NAME_LEN);
    if ((sscanf(body, location_fmt,
        chatserver_name, tcp_port, udp_port) != 3))
    {
        /* We didn't get the three things we were looking for
         * back from the server.
         */
        fprintf(stderr, "%s: unexpected response from server\n", __func__);
        return EPROTO; /* protocol error */
    }

    debug_sub_print(DBG_LOC, "%s: returning\n", __func__);
    /* 5. Clean up after ourselves and return. */
    free(buf);
    return 0;

}

/* Performs _retrieve_chaterver_info() while trying to cope with
 * some transient failures.
 *
 * Retries are governed by macros RETRY_COUNT and RETRY_PAUSE
 * (with exponential backoff on subsequent retries)
 *
 * THIS IS BASICALLY A COPY OF retry_handler ADAPTER FOR THE
 * SIGNATURE OF _retrieve_chatserver_info. IT WOULD BE NICE TO
 * REFACTOR THEM ALL TOGETHER BUT I AM NOT SUPER FAMILIAR WITH
 * FUNCTION POINTERS AND ARGUMENT LISTS.
 */
int retrieve_chatserver_info(char *chatserver_name,
    u_int16_t *tcp_port, u_int16_t *udp_port)
{
    int result = -1,
        retries = RETRY_COUNT,
        pause = RETRY_PAUSE;

    /* Bump the number of retries to accomodate the first
     * run through the loop (0 retries == 1 execution) */
    retries++;
    while ((retries-- >=0) && 
        (result =
        _retrieve_chatserver_info(chatserver_name, tcp_port, udp_port))
        != 0) {

        switch (result) {
            case 0: /* success */
                return 0;
            case_RETRYABLE /* See client.h */
                /* Retry */
                //fprintf(stderr, ".");
                sleep(pause);
                pause *= 2;
                break;
            default:
                /* Don't know what to do with these cases */
                //fprintf(stderr, "\n");
                debug_sub_print(DBG_FAULT, "%s: default: %d\n", 
                    __func__, result);
                return result;
                break;
        }
    }

    return result;
}

/* Wrap the handle_XXX functions with some retrying logic
 * this is intended only to cover some transient network
 * errors. Logic specific to the handlers must be implemented
 * elsewhere.
 *
 * This is a bit dicey, but most of the functions we are
 * interested in either take a single char* argument or
 * none at all. Here we assume that if *arg is NULL,
 * the function is one of those with no arguments.
 *
 * Retries are governed by (number of) retries and pause (in
 * seconds) with exponential backoff on successive retries.
 */
int retry_handler(int (*handler)(char*), char *arg,
    int *retries, int *pause)
{

    int result = -1;

    /* Bump the number of retries to accomodate the first
     * run through the loop (0 retries == 1 execution) */
    (*retries)++;
    while ((*retries)-- >=0) {

        /* Make call */
        if (arg) {
            result = handler(arg);
        } else {
            /* Cast to no-args and call */
            result = ((int (*)())handler)();
        }

        switch (result) {
            case 0: /* success */
                return 0;
            case_RETRYABLE /* See client.h */
                /* Retry */
                //fprintf(stderr, ".");
                sleep(*pause);
                (*pause) *= 2;
                break;
            default:
                /* Don't know what to do with these cases */
                //fprintf(stderr, "\n");
                debug_sub_print(DBG_FAULT, "%s: default: %d\n", 
                    __func__, result);
                return result;
                break;
        }

    }

    return result;
}



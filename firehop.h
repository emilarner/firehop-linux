#ifndef FIREHOP_H
#define FIREHOP_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <sys/signal.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>

#include "config.h"
#include "available.h"


#define local_exit(msg) do {    \
    perror(msg);                \
    free(pair);                    \
    return NULL;                \
    } while (0);                \


enum UDPSpecial
{
    UninitializedPort = 0
};


/* A structure containing resolutions of a source port to the source port of the other UDP client. */
struct UDPMode
{
    struct sockaddr_in *remote2local[65536];
    struct sockaddr_in *local2remote[65536];
};

/* Modes that the program can be within. */
enum Modes
{
    UndefinedMode,
    TCPMode,
    UDPMode
};

/* The main object of this program, containing everything that is needed. */
struct Firehop
{
    Available *queue;
    enum Modes mode;
    struct UDPMode *udpmode; 


    bool whitelist_mode;
    in_addr_t *wlist_ips;
    size_t wlist_ips_len; 

    size_t max;
    int cfd;

    uint16_t cport;
    uint16_t rport;
    uint16_t lport;
};

typedef struct Firehop Firehop;

/* A pair of two file descriptors that shall be binded/glued together by Firehop. */ 
struct Pair
{
    Firehop *fh; 

    int remotefd;
    int localfd;
};

void alog_print(FILE *stream, const char *msg);

#define log_print(x) alog_print(stdout, x);
#define elog_print(x) alog_print(stderr, x);

/* Initialize Firehop. */
Firehop *firehop(int cport, int rport, int lport, enum Modes mode);

/* Start the Firehop server. */
void firehop_start(Firehop *f);

/* Shut down the Firehop server, freeing its many allocated resources. */
void firehop_free(Firehop *f);

/* Create a TCP server, given a port. */
int create_server(uint16_t port);

/* Create a UDP 'server', given a port. */
int create_udp_server(uint16_t port);

/* The thread for handling the control connection. */
void *control(Firehop *fh);

/* Receive all, send all. */
/* sendfile() wouldn't work here, since it requires fds that are mmap-able, and sockets aren't. */
bool recvallsendall();

/* Handle local connections. */
void *handle_local(void *f);
void *local(void *f);

/* Bind both a local and remote client together, given a struct Pair* casted to a void* */
void *glue(void *p);

/* Handle remote connections. */
void *handle_remote(void *f);
void *remote(void *f);

/* UDP stuff. */
void *udp_remote(void *f);
void *udp_local(void *f);



#endif
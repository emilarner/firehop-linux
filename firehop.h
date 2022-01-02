#ifndef FIREHOP_H
#define FIREHOP_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

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



enum Modes
{
    UndefinedMode,
    TCPMode,
    UDPMode
};

struct Firehop
{
    Available *queue;

    int cfd;

    uint16_t cport;
    uint16_t rport;
    uint16_t lport;
};

typedef struct Firehop Firehop;


struct Pair
{
    Firehop *fh; 

    int remotefd;
    int localfd;
};

void dprint(const char *msg); 

Firehop *firehop(int cport, int rport, int lport);
void firehop_start(Firehop *f);

int create_server(uint16_t port);

void *control(Firehop *fh);

bool recvallsendall();

void *handle_local(void *f);
void *local(void *f);

void *glue(void *p);

void *handle_remote(void *f);
void *remote(void *f);



#endif
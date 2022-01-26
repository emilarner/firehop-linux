#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>
#include <sys/select.h>

/* You probably do not want to change anything in here, unless you know what you're doing. */

#define SYSTEM "Linux"
#define VERSION "1.1"

/* Don't change. */
#define DATAGRAM_MAX 65536

/* Change, if you will. */
#define SELECT_TIMEOUT_SEC 900

/* Don't change. */
#define MAX_FDS 1023

/* Don't change. */
#define RECV_BUFFERSIZE 4096

/* listen() backlog. */
#define BACKLOG 64

/* Max amount of whitelisted IPs on stack memory? */
#define MAX_WLISTIPS 256

/* The strftime() format for the log printing. */
#define TIME_FORMAT "%m/%d/%y %H:%M"

/* Checking against a critical error. */
#if MAX_FDS >= FD_SETSIZE
    #error "select() cannot handle file descriptors greater than or equal to 1024"
#endif

#endif
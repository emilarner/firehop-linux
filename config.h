#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>
#include <sys/select.h>

#define VERSION "1.0b"
#define FHDEBUG true

#define SELECT_TIMEOUT_SEC 900
#define MAX_FDS 1023
#define RECV_BUFFERSIZE 4096


#if MAX_FDS >= FD_SETSIZE
    #error "select() cannot handle file descriptors greater than or equal to 1024"
#endif

#endif
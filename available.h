#ifndef AVAILABLE_H
#define AVAILABLE_H

#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

struct Available
{
    int *fds;
    size_t capacity;
    size_t length;
};

typedef struct Available Available;

Available *init_available();
size_t available_push(Available *a, int fd);
int available_pop(Available *a, size_t index);
int available_get(Available *a, size_t index);
void available_free(Available *a);

#endif
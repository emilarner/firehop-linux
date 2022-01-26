/* available.h - a dynamic list for long types and pointers that is LIFO. */ 

#ifndef AVAILABLE_H
#define AVAILABLE_H

#define INITIAL_CAPACITY 16

#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

struct Available
{
    long *fds;
    size_t capacity;
    size_t length;
};

/* Do not require the prefix 'struct' when addressing this object. */
typedef struct Available Available;

/* Initialize an Available object. */
Available *init_available();

/* Push a long onto the Available stack. */
size_t available_push(Available *a, long fd);

/* Pop the last value off of the Available stack. Returns (signed long) -1 on failure. */
long available_pop(Available *a, size_t index);

/* Get a value at a specific index. -1 on failure. */
long available_get(Available *a, size_t index);

/* Free the resources of an Available object. */
void available_free(Available *a);

#endif
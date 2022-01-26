#include "available.h"

Available *init_available()
{
    Available *a = (Available*) malloc(sizeof(*a));

    /* Capacity, so that we can exponentially allocate. */
    a->capacity = INITIAL_CAPACITY;
    a->fds = (long*) malloc(INITIAL_CAPACITY * sizeof(long));
    a->length = 0;

    return a;
}

size_t available_push(Available *a, long fd)
{
    /* When it is apparent that we will run out of memory, allocate more. */
    if (a->length == a->capacity)
    {
        /* Allocate more by doubling the capacity, thereby having exponential growth. */
        a->capacity *= 2;
        a->fds = (long*) realloc(a->fds, a->capacity * sizeof(long));
    }


    a->fds[a->length] = fd;
    return a->length++; 
}


long available_get(Available *a, size_t index)
{
    return a->fds[index];
}

long available_pop(Available *a, size_t index)
{
    if (a->length == 0)
        return -1;

    /* LIFO algorithm. */
    size_t stackindex = a->length - 1 - index; 
    long fd = a->fds[stackindex];
    a->length--;

    return fd;
}

void available_free(Available *a)
{
    free(a->fds);
    free(a); 
}

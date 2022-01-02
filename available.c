#include "available.h"

Available *init_available()
{
    Available *a = (Available*) malloc(sizeof(*a));

    a->capacity = 12;
    a->fds = (int*) malloc(a->capacity * sizeof(int));
    a->length = 0;

    return a;
}

size_t available_push(Available *a, int fd)
{
    if (a->length == a->capacity)
    {
        a->capacity *= 2;
        a->fds = (int*) realloc(a->fds, a->capacity * sizeof(int));
    }


    a->fds[a->length] = fd;
    return a->length++; 
}


int available_get(Available *a, size_t index)
{
    return a->fds[index];
}

int available_pop(Available *a, size_t index)
{
    size_t stackindex = a->length - 1 - index; 
    int fd = a->fds[stackindex];
    a->length--;

    return fd;
}

void available_free(Available *a)
{
    free(a->fds);
    free(a); 
}

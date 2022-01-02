#include "firehop.h"

void dprint(const char *msg)
{
    if (FHDEBUG)
        printf("Debug: %s\n", msg);
}

Firehop *firehop(int cport, int rport, int lport)
{
    Firehop *f = (Firehop*) malloc(sizeof(*f));

    f->queue = init_available();

    f->cport = htons(cport);
    f->rport = htons(rport);
    f->lport = htons(lport);

    f->cfd = 0;

    return f;
}

void firehop_start(Firehop *f)
{
    pthread_t premote;
    pthread_t plocal;

    pthread_create(&premote, NULL, &remote, (void*) f);
    pthread_create(&plocal, NULL, &local, (void*) f);

    control(f);
}

int create_server(uint16_t port)
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);

    int option = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

    struct sockaddr_in addr;

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = port;

    if (bind(fd, (struct sockaddr*) &addr, (socklen_t) sizeof(addr)) < 0)
    {
        fprintf(stderr, "%s:%d", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
        perror(" failed to bind()");
        return -1;
    }

    listen(fd, 64);

    return fd;
}



void *control(Firehop *fh)
{
    int fd = create_server(fh->cport);

    if (fd < 0)
    {
        perror("failed to create control server");
        return NULL;
    }

    struct sockaddr_in caddr;
    socklen_t caddr_len = sizeof(caddr);

    fh->cfd = accept(fd, (struct sockaddr*) &caddr, &caddr_len);

    if (fh->cfd < 0)
    {
        perror("accept() failed in control()");
        return NULL;
    }

    if (FHDEBUG)
    {
        printf("Debug: control connection from %s:%d\n", inet_ntoa(caddr.sin_addr), 
                                                        ntohs(caddr.sin_port));
    }

    while (true)
    {
        getc(stdin);
        usleep(750000);
    }
}

bool recvallsendall(int from, int to)
{
    char buffer[RECV_BUFFERSIZE];
    ssize_t length = 0;

    while ((length = recv(from, buffer, sizeof(buffer), MSG_DONTWAIT)) != -1)
    {
        if (length == 0)
        {
            printf("Disconnected\n");
            return false;
        }

        send(to, buffer, length, 0);
    }

    return true;
}

void *glue(void *p)
{
    struct Pair *pair = (struct Pair*) p; 

    /* Specify a timeout of 5 minutes. */ 
    struct timeval timeout;
    timeout.tv_sec = SELECT_TIMEOUT_SEC;
    timeout.tv_usec = 0;

    fd_set fds;

    while (true)
    {
        FD_ZERO(&fds);
        FD_SET(pair->localfd, &fds);
        FD_SET(pair->remotefd, &fds);

        int status = select(MAX_FDS, &fds, NULL, NULL, &timeout);

        /* Status is -1 and errno is set. */ 
        if (status == -1)
            local_exit("select failed on handle_local()");
        
        /* If status is 0, select() timed out. */ 
        if (status == 0)
            local_exit("select() timed out.");

        /* The remote connection has sent data. */
        if (FD_ISSET(pair->localfd, &fds))
        {
            if (!recvallsendall(pair->localfd, pair->remotefd))
            {
                free(pair);
                return NULL;
            }
        }

        /* The local connection has sent data. */ 
        if (FD_ISSET(pair->remotefd, &fds))
        {
            if (!recvallsendall(pair->remotefd, pair->localfd))
            {
                free(pair);
                return NULL;
            }
        }

    }
}

void *local(void *f)
{
    Firehop *fh = (Firehop*) f;
    int fd = create_server(fh->lport);

    if (fd < 0)
    {
        perror("failed to create server in local()");
        return NULL;
    }

    while (true)
    {
        struct sockaddr_in caddr;
        socklen_t clen = sizeof(caddr);

        int cfd = accept(fd, (struct sockaddr*) &caddr, &clen);

        if (!cfd)
        {
            perror("local accept() failed");
            return NULL;
        }

        if (FHDEBUG)
            printf("Debug: local connection from %s:%d\n", inet_ntoa(caddr.sin_addr), 
                                                            ntohs(caddr.sin_port));

        available_push(fh->queue, cfd);
        send(fh->cfd, "CONNECT", sizeof("CONNECT") - 1, 0); 
    }
}

void *remote(void *f)
{
    Firehop *fh = (Firehop*) f;
    int fd = create_server(fh->rport);

    if (fd < 0)
    {
        perror("failed to create server in remote()");
        return NULL;
    }

    while (true)
    {
        struct sockaddr_in caddr;
        socklen_t clen = sizeof(caddr);

        int cfd = accept(fd, (struct sockaddr*) &caddr, &clen);

        if (!cfd)
        {
            perror("remote accept() failed");
            return NULL;
        }

        if (FHDEBUG)
            printf("Debug: remote connection from %s:%d\n", inet_ntoa(caddr.sin_addr), 
                                                            ntohs(caddr.sin_port));

        struct Pair *p = (struct Pair*) malloc(sizeof(*p));

        p->remotefd = cfd;
        p->localfd = available_pop(fh->queue, 0);
        p->fh = fh;

        pthread_t tmp;
        pthread_create(&tmp, NULL, &glue, (void*) p); 
    }
}
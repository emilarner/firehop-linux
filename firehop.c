#include "firehop.h"

void alog_print(FILE *stream, const char *msg)
{
    char time_format[32];

    time_t current;
    time(&current);
    strftime(time_format, sizeof(time_format), TIME_FORMAT, localtime(&current));

    fprintf(stream, "[%s]: %s\n", time_format, msg);
}

Firehop *firehop(int cport, int rport, int lport, enum Modes mode)
{
    /* Check if the ports are out of range, which is important. */
    if ((uint32_t) cport > 65535 || (uint32_t) rport > 65535 || (uint32_t) lport > 65535)
    {
        fprintf(stderr, "Critical error: ports cannot be above 65535!\n");
        return NULL; 
    }

    Firehop *f = (Firehop*) malloc(sizeof(*f));

    f->queue = init_available();
    f->mode = mode;
    f->udpmode = NULL; 

    /* Convert from host byte order to network byte order for a uint16_t. */
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

    if (f->mode == UDPMode)
    {
        log_print("Firehop is starting up in UDP mode, which as of now uses a lot more memory!");

        f->udpmode = (struct UDPMode*) calloc(sizeof(struct UDPMode), 1);

        pthread_create(&premote, NULL, &udp_remote, (void*) f);
        pthread_create(&plocal, NULL, &udp_local, (void*) f);
    }
    else 
    {
        log_print("Firehop is starting up in its regular TCP mode.");

        pthread_create(&premote, NULL, &remote, (void*) f);
        pthread_create(&plocal, NULL, &local, (void*) f);
    }

    control(f);
}

void firehop_free(Firehop *f)
{
    log_print("Freeing Firehop and closing its file descriptors...");

    if (f->mode == UDPMode)
    {
        if (f->udpmode != NULL)
            free(f->udpmode);
    }
    else
    {
        if (f->queue != NULL)
            available_free(f->queue);
    }

    close(f->cfd);
    free(f);
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
        fprintf(stderr, "While trying to bind %s:%d, we received an error: %s\n ", 
                        inet_ntoa(addr.sin_addr), ntohs(addr.sin_port), strerror(errno));
            
        return -1;
    }

    listen(fd, BACKLOG);
    return fd;
}



void *control(Firehop *fh)
{
    int fd = create_server(fh->cport);

    if (fd < 0)
    {
        perror("Failed to create control server");
        return NULL;
    }

    struct sockaddr_in caddr;
    socklen_t caddr_len = sizeof(caddr);

    fh->cfd = accept(fd, (struct sockaddr*) &caddr, &caddr_len);

    if (fh->cfd < 0)
    {
        perror("accept() failed in control server");
        return NULL;
    }

    log_print("Connection accepted from control server.");
    printf("     ->%s:%d\n", inet_ntoa(caddr.sin_addr), ntohs(caddr.sin_port));

    /* Infinite loop to keep the program running. getc() will block. */
    while (true)
        getc(stdin);
}

bool recvallsendall(int from, int to)
{
    char buffer[RECV_BUFFERSIZE];
    ssize_t length = 0;

    while ((length = recv(from, buffer, sizeof(buffer), MSG_DONTWAIT)) != -1)
    {
        if (length == 0)
            return false;

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
        perror("Failed to create local server");
        return NULL;
    }

    while (true)
    {
        struct sockaddr_in caddr;
        socklen_t clen = sizeof(caddr);

        int cfd = accept(fd, (struct sockaddr*) &caddr, &clen);

        if (!cfd)
        {
            perror("accept() failed in local server");
            return NULL;
        }

        log_print("Connection accepted from local server");
        printf("    ->Address: %s:%d\n", inet_ntoa(caddr.sin_addr), ntohs(caddr.sin_port));

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
        perror("Failed to create the remote server");
        return NULL;
    }

    while (true)
    {
        struct sockaddr_in caddr;
        socklen_t clen = sizeof(caddr);
        int other = 0;

        int cfd = accept(fd, (struct sockaddr*) &caddr, &clen);

        if (!cfd)
        {
            perror("accept() failed in the remote server");
            return NULL;
        }

        log_print("Connection accepted from remote server");
        printf("    ->Address: %s:%d\n", inet_ntoa(caddr.sin_addr), ntohs(caddr.sin_port));

        if ((other = available_pop(fh->queue, 0)) == -1)
        {
            elog_print("Error: we have a remote connection, but no corresponding local one!")
            close(cfd);
            continue;
        }

        struct Pair *p = (struct Pair*) malloc(sizeof(*p));

        p->remotefd = cfd;
        p->localfd = other; 
        p->fh = fh;

        pthread_t tmp;
        pthread_create(&tmp, NULL, &glue, (void*) p); 

    }
}

int create_udp_server(uint16_t port)
{
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    
    struct sockaddr_in saddr;
    saddr.sin_family = AF_INET;
    saddr.sin_port = port;
    saddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sockfd, (struct sockaddr*) &saddr, (socklen_t) sizeof(saddr)) < 0)
    {
        elog_print("Binding UDP server failed!");
        fprintf(stderr, "   ->For port %d, %s\n", port, strerror(errno));
        return -1;
    }

    listen(sockfd, BACKLOG);
    return sockfd;
}

void *udp_remote(void *f)
{
    Firehop *fh = (Firehop*) f;
    int sockfd = create_udp_server(fh->rport);

    while (true)
    {
        char msg[DATAGRAM_MAX];
        ssize_t msg_length = 0;

        struct sockaddr_in client;
        socklen_t client_length;

        if ((msg_length = recvfrom(sockfd, msg, sizeof(msg_length), 0, 
                        (struct sockaddr*) &client, &client_length)) == -1)
        {

        }

        uint16_t uport = client.sin_port;

        if (fh->udpmode->remote2local[uport] == NULL)
        {
            struct sockaddr_in *luaddr = (struct sockaddr_in*) available_pop(fh->queue, 0);

            if ((long) luaddr == -1)
            {
                elog_print("Error: remote connection received, but no corresponding local connection!");
                continue; 
            }

            struct sockaddr_in *ruaddr = (struct sockaddr_in*) malloc(sizeof(*ruaddr));
            *ruaddr = client; 

            fh->udpmode->remote2local[uport] = luaddr;
            fh->udpmode->local2remote[luaddr->sin_port] = ruaddr;
        }

        if (sendto(sockfd, msg, sizeof(msg_length), 0, (struct sockaddr*) fh->udpmode->remote2local[uport], 
            (socklen_t) sizeof(struct sockaddr_in)) == -1)
        {
            uint16_t other = fh->udpmode->remote2local[client.sin_port]->sin_port;
            free(fh->udpmode->local2remote[other]);
            fh->udpmode->local2remote[other] = NULL;
            free(fh->udpmode->remote2local[client.sin_port]);
            fh->udpmode->remote2local[client.sin_port] = NULL; 
        }
        
    }

}

void *udp_local(void *f)
{
    Firehop *fh = (Firehop*) f;
    int sockfd = create_udp_server(fh->lport);

    while (true)
    {
        struct sockaddr_in client;
        socklen_t client_length;

        char msg[DATAGRAM_MAX];
        ssize_t msg_length;

        if ((msg_length = recvfrom(sockfd, msg, sizeof(msg), 0, 
                        (struct sockaddr*) &client, &client_length)) == -1)
        {
            elog_print("Error while receiving UDP datagrams on local server!");
            fprintf(stderr, "   ->%s\n", strerror(errno));
            return NULL;
        }

        uint16_t uport = client.sin_port;

        if (fh->udpmode->local2remote[uport] == NULL)
        {
            struct sockaddr_in *uaddr = (struct sockaddr_in*) malloc(sizeof(*uaddr));
            *uaddr = client;


            available_push(fh->queue, (long) uaddr);
            send(fh->cport, "CONNECT", sizeof("CONNECT") - 1, 0);
            continue;
        }

        if (sendto(sockfd, msg, msg_length, 0, (struct sockaddr*) fh->udpmode->local2remote[uport], 
                                            (socklen_t) sizeof(struct sockaddr_in)) == -1)
        {
            uint16_t other = fh->udpmode->local2remote[uport]->sin_port;
            free(fh->udpmode->remote2local[other]);
            fh->udpmode->remote2local[other] = NULL;
            free(fh->udpmode->local2remote[uport]);
            fh->udpmode->local2remote[uport] = NULL;
        }

    }

}
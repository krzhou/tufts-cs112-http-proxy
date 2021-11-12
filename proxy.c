/**************************************************************
*
*                            proxy.c
*
*     Final Project: High Performance HTTP Proxy
*     Author:  Keren Zhou (kzhou), Ruiyuan Gu (rgu03)
*     Date: 2021-11-10
*
*     Summary:
*     Main driver for HTTP proxy.
*
**************************************************************/

#include "logger.h"
#include "cache.h"
#include "sock_buf.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#define BUF_SIZE 1024

static int listen_port; /* Port that proxy listens on. */
static int listen_sock; /* Listening socket of the proxy. */
static fd_set active_fd_set; /* FD sets of all active sockets. */
static fd_set read_fd_set;   /* FD sets of all sockets read to be read. */

/**
 * @brief Initialzed a listening socket that listens on the given port.
 *
 * @param port Port to listen on.
 */
int init_listen_sock(int port)
{
    int sock;
    int optval;
    struct sockaddr_in addr;

    /* Create a socket. */
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        PLOG_FATAL("socket");
    }

    /* Allow reuse of local addresses. */
    optval = 1;
    if (setsockopt(sock, SOL_SOCKET,
                   SO_REUSEADDR,
                   (const void *)&optval,
                   sizeof(int)) < 0) {
        PLOG_FATAL("setsockopt");
    }

    /* Build the sock's internet address. */
    addr.sin_family = AF_INET; /* Use the Internet. */
    addr.sin_port = htons((unsigned short)port); /* Port to listen. */
    addr.sin_addr.s_addr = htonl(INADDR_ANY); /* Let the system figure out our
                                               * IP address. */

    /* Associate the proxy socket with the given IP address and port. */
    if (bind(sock,
             (struct sockaddr*)&addr,
             sizeof(addr)) < 0) {
        PLOG_FATAL("bind");
    }

    return sock;
}

/**
 * @brief Initialize the proxy.
 */
void init_proxy(void)
{
    /* Setup listening socket. */
    listen_sock = init_listen_sock(listen_port);
    if (listen(listen_sock, 1) < 0) {
        PLOG_FATAL("listen");
    }
    LOG_INFO("listen on port %d", listen_port);

    /* Init FD set for select(). */
    FD_ZERO(&active_fd_set);
    FD_SET(listen_sock, &active_fd_set);

    /* Init LRU cache. */
    cache_init(10);

    /* Init socket buffer array. */
    sock_buf_arr_init();

}

/**
 * @brief Free all the proxy resource.
 */
void clear_proxy(void)
{
    /* Close all sockets. */
    for (int fd = 0; fd < FD_SETSIZE; ++fd) {
        if (FD_ISSET(fd, &active_fd_set)) {
            close(fd);
        }
    }
    close(listen_sock);

    /* Free LRU cache. */
    cache_clear();

    /* Free socket buffer array. */
    sock_buf_arr_clear();
}

/**
 * @brief SIGINT handler that cleans up proxy and exit.
 */
void INT_handler(int sig)
{
    (void)sig;
    clear_proxy();
    LOG_INFO("shut down");
    exit(EXIT_SUCCESS);
}

/**
 * @brief Accept a new client.
 */
void accept_client(void)
{
    int client_sock; /* FD for client sockect. */
    struct sockaddr_in client_addr; /* Client address. */
    unsigned size = sizeof(client_addr);

    client_sock = accept(listen_sock,
                        (struct sockaddr *)&client_addr,
                        &size);
    if (client_sock < 0) {
        PLOG_ERROR("accept");
        /* Ignore this client. */
        return;
    }

    /* Add new client to selection FD set. */
    FD_SET(client_sock, &active_fd_set);

    LOG_INFO("accept %s:%hu",
             inet_ntoa(client_addr.sin_addr),
             ntohs(client_addr.sin_port));
}

/**
 * @brief Disconnect a client of the given FD.
 *
 * @param fd FD for client socket.
 */
void disconnect_client(int fd)
{
    /* Close connection with client. */
    close(fd);

    /* Remove the client from FD set for select(). */
    FD_CLR(fd, &active_fd_set);

    LOG_INFO("disconnect client (fd: %d), fd");
}

int main(int argc, char** argv)
{
    char* buf = NULL;
    int n;

    /* Parse cmd line args. */
    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>", argv[0]);
        exit(EXIT_FAILURE);
    }
    listen_port = atoi(argv[1]);

    init_proxy();

    /* Clean up and stop proxy by CTRL+C. */
    signal(SIGINT, INT_handler);

    /* Main loop. */
    while(true) {
        /* Block until input arrives on one or more active sockets. */
        read_fd_set = active_fd_set;
        if (select(FD_SETSIZE, &read_fd_set, NULL, NULL, NULL) < 0) {
            PLOG_FATAL("select");
        }
        for (int fd = 0; fd < FD_SETSIZE; ++fd) {
            if (FD_ISSET(fd, &read_fd_set)) {
                LOG_INFO("fd: %d", fd); /* TEST */

                /* Server accepts a new connection from client. */
                if (fd == listen_sock) {
                    accept_client();
                }
                /* Handle arriving data from a connected socket. */
                else {
                    /* TEST: Echo client message. */

                    /* Init message buffer. */
                    buf = malloc(BUF_SIZE);
                    if (buf == NULL) {
                        PLOG_ERROR("malloc");
                    }

                    /* Read input string from the client. */
                    bzero(buf, BUF_SIZE);
                    n = read(fd, buf, BUF_SIZE);
                    if (n < 0) {
                        PLOG_ERROR("read");
                    }
                    fprintf(stderr, "receive %d bytes: %s", n, buf);
                    
                    /* Echo the input string back to the client */
                    n = write(fd, buf, n);
                    if (n < 0) {
                        PLOG_ERROR("write");
                    }

                    disconnect_client(fd);

                    /* Cleanup message buffer. */
                    free(buf);
                    buf = NULL;
                }
            }
        }
    }

    clear_proxy();
    LOG_INFO("shut down");

    return EXIT_SUCCESS;
}

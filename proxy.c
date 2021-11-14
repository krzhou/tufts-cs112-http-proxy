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

#include "cache.h"
#include "http_parser.h"
#include "logger.h"
#include "sock_buf.h"
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#define BUF_SIZE 8192

static int listen_port; /* Port that proxy listens on. */
static int listen_sock; /* Listening socket of the proxy. */
static fd_set active_fd_set; /* FD sets of all active sockets. */
static fd_set read_fd_set;   /* FD sets of all sockets read to be read. */
static int max_fd = 4; /* Largest used FD so far. */

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
            FD_CLR(fd, &active_fd_set);
            close(fd);
        }
    }

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

    /* Create socket buffer for this new client. */
    if (sock_buf_add_client(client_sock) == 0) {
        LOG_ERROR("fail to add client socket buffer");
        close(client_sock);
        return;
    }

    /* Update upperbound of used FD for sockets. */
    if (client_sock > max_fd) {
        max_fd = client_sock;
    }

    /* Add new client to selection FD set. */
    FD_SET(client_sock, &active_fd_set);

    LOG_INFO("accept %s:%hu",
             inet_ntoa(client_addr.sin_addr),
             ntohs(client_addr.sin_port));
}

/**
 * Connect to server by the given hostname and port.
 *
 * @param hostname Server hostname without port number.
 * @param port Server port number.
 * @param client_sock FD for client socket.
 * @return Socket of the new connected client.
 */
int connect_server(const char* hostname, const int port, int client_sock)
{
    int server_sock;
    struct hostent* server;
    struct sockaddr_in server_addr;

    /* Create server socket. */
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        PLOG_FATAL("socket");
    }

    /* Get the server's DNS entry. */
    server = gethostbyname(hostname);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host as %s\n", hostname);
        exit(EXIT_FAILURE);
    }

    /* Build the server's Internet address. */
    bzero((char*) &server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    bcopy((char*)server->h_addr,
          (char*)&server_addr.sin_addr.s_addr,
          server->h_length);
    server_addr.sin_port = htons(port);

    /* Create a connection with the server. */
    if (connect(server_sock,
                (struct sockaddr*)&server_addr,
                sizeof(server_addr)) < 0) {
        PLOG_FATAL("connect");
    }

    /* Update upperbound of used FD for sockets. */
    if (server_sock > max_fd) {
        max_fd = server_sock;
    }

    /* Add new server to selection FD set. */
    FD_SET(server_sock, &active_fd_set);

    /* Create socket buffer for this server. */
    sock_buf_add_server(server_sock, client_sock);

    LOG_INFO("connect to %s:%d", hostname, port);

    return server_sock;
}

/**
 * @brief Disconnect the given server.
 * 
 * @param fd FD for server socket.
 */
void disconnect_server(int fd)
{
    /* Close connection with server. */
    close(fd);

    /* Remove the server from FD set for select(). */
    FD_CLR(fd, &active_fd_set);

    /* Remove socket buffer for the server. */
    sock_buf_rm(fd);

    LOG_INFO("disconnect server (fd: %d)", fd);
}

/**
 * @brief Disconnect a client of the given FD.
 *
 * Its required servers will also be disconnected.
 * @param fd FD for client socket.
 */
void disconnect_client(int fd)
{
    struct sock_buf* sock_buf = NULL;

    /* Close connection with client. */
    close(fd);

    /* Remove the client from FD set for select(). */
    FD_CLR(fd, &active_fd_set);

    /* Remove socket buffer of the client. */
    sock_buf_rm(fd);

    /* Remove socket buffer of its requested servers. */
    for (int i = 0; i < FD_SETSIZE; ++i) {
        sock_buf = sock_buf_get(i);
        if (sock_buf != NULL && sock_buf->client == fd) {
            disconnect_server(i);
        }
    }

    LOG_INFO("disconnect client (fd: %d)", fd);
}


/**
 * @brief Handle one client at a time.
 * 
 * @param fd FD for client socket.
 */
void handle_one_client(int fd)
{
    int server_sock;
    char buf[BUF_SIZE]; /* Message buffer. */
    int n; /* Byte size actually received or sent. */
    char* request_head = NULL; /* Head of client request without the empty
                                * line. */
    int request_head_len = -1;
    char* request_body = NULL; /* Body entity of client request. */
    int request_body_len = -1;
    char* response_head = NULL; /* Head of server response without the empty
                                 * line. */
    int response_head_len = -1;
    char* response_body = NULL; /* Body entity of server response. */
    int response_body_len = -1;
    char* method = NULL; /* Method field in client request. */
    char* url = NULL; /* URL field in client request. */
    char* request_version = NULL; /* Version field in client request. */
    char* host = NULL; /* Host field in client request. */
    char* hostname = NULL; /* Server hostname without port number. */
    int port = 80; /* Server port in client request. 80 by default. */
    char* response_version = NULL; /* Version field in server response. */
    int status_code = -1; /* Status code in server response. */
    char* phrase = NULL; /* Phrase field in server response. */
    int content_length = -1; /* Content-Length field in server response. */
    char* cache_control = NULL; /* Cache-Control field in server response. */
    int max_age = 3600; /* Time-to-live in seconds for a cache element.
                         * One hour by default. */
    bool cache_hit = 0; /* Whether response is cached. */
    char* val = NULL; /* Cache value, i.e. head + "\r\n" + body. */
    int val_len = 0; /* Byte size of cache value. */
    int age = 0; /* Age of cache element in seconds. */
    char* key = NULL;

    /* Receive client request. */
    bzero(buf, BUF_SIZE);
    n = read(fd, buf, BUF_SIZE);
    if (n < 0) {
        PLOG_FATAL("read");
    }
    /* Assume that the whole request head is received in one read. */

    /* Parse client request. */
    parse_body_head(buf,
                    n,
                    &request_head,
                    &request_head_len,
                    &request_body,
                    &request_body_len);
    parse_request_head(request_head, &method, &url, &request_version, &host);
    parse_host_field(host, &hostname, &port);
    LOG_INFO("parse client request:\n"
             "- method: %s\n"
             "- url: %s\n"
             "- version: %s\n"
             "- hostname: %s\n"
             "- port number: %d\n\n",
             method,
             url,
             request_version,
             hostname,
             port);

    /* Use hostname + url as cache key. */
    key = malloc(strlen(hostname) + strlen(url) + 1);
    if (key == NULL) {
        PLOG_FATAL("malloc");
    }
    strcpy(key, hostname);
    strcat(key, url);

    /* Get server response from cache. */
    cache_hit = (cache_get(key, &val, &val_len, &age) > 0);
    if (cache_hit) {
        LOG_INFO("cache hit\n");

        /* Get head and body from val. */
        parse_body_head(val,
                        val_len,
                        &response_head,
                        &response_head_len,
                        &response_body,
                        &response_body_len);
        parse_response_head(response_head,
                            &response_version,
                            &status_code,
                            &phrase,
                            &content_length,
                            &cache_control);
        LOG_INFO("parse server response:\n"
                 "- version: %s\n"
                 "- status code: %d\n"
                 "- phrase: %s\n"
                 "- content length: %d\n"
                 "- cache control: %s\n\n",
                 response_version,
                 status_code,
                 phrase,
                 content_length,
                 cache_control);
        parse_cache_control(cache_control, &max_age);
        LOG_INFO("parse cache control: max-age=%d\n\n", max_age);

        /* Append age header line to head. */
        response_head = realloc(response_head, response_head_len + 100);
        sprintf(response_head + response_head_len, "Age: %d\r\n", age);
        response_head_len = strlen(response_head);
    }
    /* Get server response from internet. */
    else {
        LOG_INFO("cache miss\n");

        /* Connect server. */
        server_sock = connect_server(hostname, port, fd);

        /* Forward client request to server. */
        n = write(server_sock, request_head, request_head_len);
        if (n < 0) {
            PLOG_FATAL("write");
        }
        n = write(server_sock, "\r\n", strlen("\r\n")); /* Empty line. */
        if (n < 0) {
            PLOG_FATAL("write");
        }
        if (strlen(request_body) > 0) {
            n = write(server_sock, request_body, request_body_len);
            if (n < 0) {
                PLOG_FATAL("write");
            }
        }

        /* Receive server response. */
        bzero(buf, BUF_SIZE);
        n = read(server_sock, buf, BUF_SIZE);
        if (n < 0) {
            PLOG_FATAL("read");
        }
        /* Assume the head is completed in one read. */

        /* Parse server response. */
        parse_body_head(buf,
                        n,
                        &response_head,
                        &response_head_len,
                        &response_body,
                        &response_body_len);
        parse_response_head(response_head,
                            &response_version,
                            &status_code,
                            &phrase,
                            &content_length,
                            &cache_control);
        LOG_INFO("parse server response:\n"
                 "- version: %s\n"
                 "- status code: %d\n"
                 "- phrase: %s\n"
                 "- content length: %d\n"
                 "- cache control: %s\n\n",
                 response_version,
                 status_code,
                 phrase,
                 content_length,
                 cache_control);
        parse_cache_control(cache_control, &max_age);
        fprintf(stderr, "parse cache control: max-age=%d\n\n", max_age);

        /* Receive the rest of server response. */
        if (response_body_len < content_length) {
            int len; /* Byte size of unreceived response body. */
            char* end; /* End of received response body. */

            LOG_INFO("content length: %d\n"
                     "byte size of received body: %d\n",
                     content_length,
                     response_body_len);

            LOG_INFO("receiving the rest of body...\n");
            response_body = realloc(response_body, content_length);
            end = response_body + response_body_len;
            len = content_length - response_body_len;
            while (len > 0) {
                n = read(server_sock, end, len);
                if (n < 0) {
                    PLOG_FATAL("read");
                }
                /* EOF */
                else if (n == 0) {
                    break;
                }
                end += n;
                len -= n;
                response_body_len += n;
            }
            LOG_INFO("content length: %d\n"
                     "byte size of received body: %d\n",
                     content_length,
                     response_body_len);
        }

        /* Cache the response. */
        val =
            malloc(response_head_len + strlen("\r\n") + response_body_len);
        val_len = 0;
        memcpy(val, response_head, response_head_len);
        val_len += response_head_len;
        memcpy(val + val_len, "\r\n", strlen("\r\n"));
        val_len += strlen("\r\n");
        memcpy(val + val_len, response_body, response_body_len);
        val_len += response_body_len;
        cache_put(key, val, val_len, max_age);

        /* Close server socket. */
        disconnect_server(server_sock);
    }

    /* Forward server response to client. */
    n = write(fd, response_head, response_head_len);
    if (n < 0) {
        PLOG_FATAL("write");
    }
    n = write(fd, "\r\n", strlen("\r\n")); /* Empty line. */
    if (n < 0) {
        PLOG_FATAL("write");
    }
    if (strlen(response_body) > 0) {
        n = write(fd, response_body, response_body_len);
        if (n < 0) {
            PLOG_FATAL("ERROR writing to server");
        }
    }

    disconnect_client(fd);

    /* Cleanup. */
    free(request_head);
    request_head = NULL;
    free(request_body);
    request_body = NULL;
    free(response_head);
    response_head = NULL;
    free(response_body);
    response_body = NULL;
    free(method);
    method = NULL;
    free(url);
    url = NULL;
    free(request_version);
    request_version = NULL;
    free(host);
    host = NULL;
    free(hostname);
    hostname = NULL;
    free(response_version);
    response_version = NULL;
    free(phrase);
    phrase = NULL;
    free(cache_control);
    cache_control = NULL;
    free(val);
    val = NULL;
    free(key);
    key = NULL;
}

/**
 * @brief Handle incoming message from a client/server.
 * 
 * @param fd FD for a client/server socket.
 */
void handle_msg(int fd)
{
    struct sock_buf* sock_buf = NULL; /* Socket buffer. */
    char buf[BUF_SIZE]; /* Message buffer. */
    int n; /* Byte size actually received or sent. */

    sock_buf = sock_buf_get(fd);
    if (sock_buf == NULL) {
        LOG_ERROR("unknown socket %d", fd);
        return;
    }

    /* Receive message. */
    bzero(buf, BUF_SIZE);
    n = read(fd, buf, BUF_SIZE);
    if (n < 0) {
        PLOG_FATAL("read");
    }
    else if (n == 0) {
        /* Socket is disconnected on the other side. */
        if (sock_buf->client < 0) {
            /* This socket is for a client since its client socket is -1. */
            disconnect_client(fd);
        }
        else {
            /* This socket is for a server since its client socket is valid. */
            disconnect_server(fd);
        }
    }

    /* TODO: Write into socket buffer. */

    /* TODO: Parse socket buffer. */

    /* TODO: For client socket, connect a server and forward request. */

    /* TODO: For server socket, forward response to its client, then disconnect 
     * server. */
}

int main(int argc, char** argv)
{
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
        if (select(max_fd + 1, &read_fd_set, NULL, NULL, NULL) < 0) {
            PLOG_FATAL("select");
        }
        for (int fd = 0; fd <= max_fd; ++fd) {
            if (FD_ISSET(fd, &read_fd_set)) {
                /* Accept new client. */
                if (fd == listen_sock) {
                    accept_client();
                }
                else {
                    // handle_one_client(fd);
                    handle_msg(fd);
                }
            }
        }
    }

    clear_proxy();
    LOG_INFO("shut down");

    return EXIT_SUCCESS;
}

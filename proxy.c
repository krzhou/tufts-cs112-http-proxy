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
 * @brief SIGPIPE hander that ignores this signal.
 *
 * @param sig
 */
void PIPE_hander(int sig)
{
    (void)sig;
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
 * @param connected_sock Socket for the other side of a CONNECT method; -1 if 
 * method is not CONNECT.
 * @param key String for cache key, i.e. hostname + url in GET request.
 * @return Socket of the new connected server.
 */
int connect_server(const char *hostname,
                   const int port,
                   int client_sock,
                   int connected_sock,
                   char* key) {
    int server_sock;
    struct hostent *server;
    struct sockaddr_in server_addr;

    /* Create server socket. */
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        PLOG_ERROR("socket");
        return -1;
    }

    /* Get the server's DNS entry. */
    server = gethostbyname(hostname);
    if (server == NULL) {
        PLOG_ERROR("no such host as %s\n", hostname);
        return -1;
    }

    /* Build the server's Internet address. */
    bzero((char *)&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&server_addr.sin_addr.s_addr,
            server->h_length);
    server_addr.sin_port = htons(port);

    /* Create a connection with the server. */
    if (connect(server_sock, (struct sockaddr *)&server_addr,
                sizeof(server_addr)) < 0) {
        PLOG_ERROR("connect");
        return -1;
    }

    /* Create socket buffer for this server. */
    if (sock_buf_add_server(server_sock,
                            client_sock,
                            connected_sock,
                            key) == 0) {
      LOG_ERROR("fail to add server socket buffer");
      close(server_sock);
      return -1;
    }

    /* Update upperbound of used FD for sockets. */
    if (server_sock > max_fd) {
        max_fd = server_sock;
    }

    /* Add new server to selection FD set. */
    FD_SET(server_sock, &active_fd_set);

    LOG_INFO("connect to %s:%d", hostname, port);

    return server_sock;
}

void disconnect_client(int fd);

/**
 * @brief Disconnect the given server.
 * 
 * @param fd FD for server socket.
 */
void disconnect_server(int fd)
{
    int connected_sock = -1;

    close(fd);
    FD_CLR(fd, &active_fd_set);

    /* Reset client's CONNECT state. */
    connected_sock = sock_buf_get(fd)->connected_sock;

    /* Remove socket buffer for the server. */
    sock_buf_rm(fd);

    /* Clear client buffer. */
    if (connected_sock >= 0) {
        disconnect_client(connected_sock);
    }


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
 * @brief Send a connect established response to client
 * 
 * @param fd FD for a client/server socket.
 * @param version version string for HTTP request.
 *
 * @return int 0 if succeed; -1 if client is disconnected, -2 if wrote a partial message.
 */
int reply_connection_established(int fd, char *version){
    char * message = NULL;
    int n;
    int size =
        strlen(version) + strlen(" 200 Connection Established\r\n\r\n");

    message = malloc(size + 1);
    if (message == NULL) {
        PLOG_FATAL("malloc");
        
    }
    strcpy(message, version);
    strcat(message, " 200 Connection Established\r\n\r\n");
    message[size] = '\0';

    n = write(fd, message, size);
    if (n < 0) {
        PLOG_ERROR("write");
        disconnect_client(fd);
        return -1;
    }
    if (n == 0) {
        LOG_ERROR("client socket is closed on the other side");
        disconnect_client(fd);
        return -1;
    }
    if(n < size){
        LOG_ERROR("Cannot write the whole message");
        return -2;
    }

    free(message);
    message = NULL;

    return 0;
}

/**
 * @brief Handle server response. If response in buffer is completed, cache and
 * forward it to client.
 *
 * @param fd FD for server socket.
 */
void handle_server_response(int fd)
{
    char* response = NULL;
    int response_len = 0;
    int max_age = 3600;
    struct sock_buf* sock_buf = sock_buf_get(fd);
    int n;

    if (sock_buf == NULL) {
        return;
    }

    /* Extract the leading completed response. */
    if (extract_first_response(&(sock_buf->msg),
                                &(sock_buf->len),
                                &response,
                                &response_len,
                                &max_age) == 0) {
        /* Response is incomplete.*/
        return;
    }

    /* Cache response. */
    if (cache_put(sock_buf->key, response, response_len, max_age) == 0) {
        LOG_ERROR("fail to cache server response");
    }

    /* Forward response to client. */
    n = write(sock_buf->client, response, response_len);
    if (n < 0) {
        PLOG_ERROR("write");
        disconnect_client(sock_buf->client);
    }
    if (n == 0) {
        LOG_ERROR("client socket is closed on the other side");
        disconnect_client(sock_buf->client);
    }

    /* Disconnect server. */
    disconnect_server(fd);

    free(response);
    response = NULL;
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
    int is_client = 0; /* Whether this socket is for a client. */
    int is_connect = 0; /*Whether this socket is in CONNECT mode*/

    sock_buf = sock_buf_get(fd);
    if (sock_buf == NULL) {
        LOG_ERROR("unknown socket %d", fd);
        return;
    }
    is_client = sock_buf->client < 0;
    is_connect = sock_buf->connected_sock >= 0;

    /* Receive message. */
    bzero(buf, BUF_SIZE);
    n = read(fd, buf, BUF_SIZE);
    if (n < 0) {
        PLOG_FATAL("read");
    }
    else if (n == 0) {
        /* Socket is disconnected on the other side. */
        if (is_client) {
            LOG_ERROR("client socket is closed on the other side");
            disconnect_client(fd);
            return;
        }
        else {
            LOG_ERROR("server socket is closed on the other side");
            disconnect_server(fd);
            return;
        }
    }
    if (is_client) {
        LOG_INFO("received %d bytes from client (fd: %d)", n, fd);
    }
    else {
        LOG_INFO("received %d bytes from server (fd: %d)", n, fd);
    }

    /* Forward encrypted messages originated from a CONNECT method. */
    if (is_connect) {
        LOG_INFO("forwarding encrypted data from fd %d to fd %d",
                fd,
                sock_buf->connected_sock); /* TEST */
        n = write(sock_buf->connected_sock, buf, n);
        if (n < 0) {
            PLOG_ERROR("write");
            if (is_client) {
                disconnect_server(sock_buf->connected_sock);
            } else {
                disconnect_client(sock_buf->connected_sock);
            }
        }
        if (n == 0) {
            LOG_ERROR("CONNECT socket is closed on the other side");
            if (is_client) {
                disconnect_server(sock_buf->connected_sock);
            }
            else {
                disconnect_client(sock_buf->connected_sock);
            }
        }
        return;
    }

    /* Write received message into socket buffer. */
    sock_buf->msg = realloc(sock_buf->msg, sock_buf->len + n);
    memcpy(sock_buf->msg + sock_buf->len, buf, n);
    sock_buf->len += n;

    /* Parse socket buffer. */
    if (is_client) {
        /* TODO: handle_client_request() */
        char* request = NULL;
        int request_len = 0;
        char* method = NULL; /* Method field in client request. */
        char* url = NULL; /* URL field in client request. */
        char* version = NULL; /* Version field in client request. */
        char* host = NULL; /* Host field in client request. */
        char* hostname = NULL; /* Server hostname without port number. */
        int port; /* Server port in client request. 80 by default. */
        int server_sock;
        char* key = NULL;

        /* Extract the leading completed request. */
        if (extract_first_request(&(sock_buf->msg),
                                  &(sock_buf->len),
                                  &request,
                                  &request_len) > 0) {
            /* Parse request. */
            parse_request_head(request, &method, &url, &version, &host);
            LOG_INFO("parsed request:\n"
                     "method: %s\n"
                     "url: %s\n"
                     "version: %s\n"
                     "host: %s\n",
                     method,
                     url,
                     version,
                     host); /* TEST */

            if (strcmp(method, "GET") == 0) {
                /* TODO: handle_get_request() */
                char* val = NULL;
                int val_len = 0;
                int age = 0;

                LOG_INFO("handle GET method");

                port = 80;
                parse_host_field(host, &hostname, &port);

                /* Use hostname + url as cache key. */
                key = malloc(strlen(hostname) + strlen(url) + 1);
                if (key == NULL) {
                    PLOG_FATAL("malloc");
                }
                strcpy(key, hostname);
                strcat(key, url);

                /* Check cache. */
                if (cache_get(key, &val, &val_len, &age) > 0) {
                    LOG_INFO("cache hit");

                    /* TODO: Add age field in the response head. */

                    /* Forward cached response to the client. */
                    n = write(fd, val, val_len);
                    if (n < 0) {
                        PLOG_ERROR("write");
                        disconnect_client(fd);
                    }
                    if (n == 0) {
                        LOG_ERROR("client socket is closed on the other side");
                        disconnect_client(fd);
                    }

                    free(method);
                    method = NULL;
                    free(url);
                    url = NULL;
                    free(version);
                    version = NULL;
                    free(host);
                    host = NULL;
                    free(hostname);
                    hostname = NULL;
                    free(key);
                    key = NULL;
                    free(request);
                    request = NULL;
                    free(val);
                    val = NULL;

                    return;
                }
                LOG_INFO("cache miss");

                /* Connect the requested server. */
                server_sock = connect_server(hostname, port, fd, -1, key);
                if (server_sock < 0) {
                    /* Fail to connect the request server. */
                    free(method);
                    method = NULL;
                    free(url);
                    url = NULL;
                    free(version);
                    version = NULL;
                    free(host);
                    host = NULL;
                    free(hostname);
                    hostname = NULL;
                    free(request);
                    request = NULL;
                    return;
                }

                /* Forward request to server. */
                n = write(server_sock, request, request_len);
                if (n < 0) {
                    PLOG_ERROR("write");
                    disconnect_server(server_sock);
                }
                if (n == 0) {
                    LOG_ERROR("server socket is closed on the other side");
                    disconnect_server(server_sock);
                }
            }
            else if (strcmp(method, "CONNECT") == 0) {
                /* handle_connect_request() */
                LOG_INFO("handle CONNECT method");
                port = 443;/*Default port for SSL link*/
                parse_host_field(host, &hostname, &port);
                
                server_sock = connect_server(hostname, port, fd, fd, NULL);
                if(server_sock < 0){
                    /*Error in connecting to server*/
                    free(method);
                    method = NULL;
                    free(url);
                    url = NULL;
                    free(version);
                    version = NULL;
                    free(host);
                    host = NULL;
                    free(hostname);
                    hostname = NULL;
                    return;
                }
                else{
                    /*Add connected_sock to client*/
                    sock_buf->connected_sock = server_sock;
                    /*Send Connection established response*/
                    reply_connection_established(fd, version);
                }
            }
            else {
                LOG_ERROR("unsupported HTTP method");
            }

            free(method);
            method = NULL;
            free(url);
            url = NULL;
            free(version);
            version = NULL;
            free(host);
            host = NULL;
            free(hostname);
            hostname = NULL;
            free(key);
            key = NULL;
            free(request);
            request = NULL;
        }
    }
    else {
        handle_server_response(fd);
    }
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

    /* Ignore SIGPIPE. */
    signal(SIGPIPE, PIPE_hander);

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
                /* Handle arriving data from a connected socket. */
                else {
                    handle_msg(fd);
                }
            }
        }
    }

    clear_proxy();
    LOG_INFO("shut down");

    return EXIT_SUCCESS;
}

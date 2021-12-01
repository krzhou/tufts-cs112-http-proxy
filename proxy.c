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
#include "http_utils.h"
#include "logger.h"
#include "sock_buf.h"
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/ssl.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#define BUF_SIZE 8192
#define CERT_FILE "cert.pem"
#define KEY_FILE "key.pem"

static int listen_port; /* Port that proxy listens on. */
static int listen_sock; /* Listening socket of the proxy. */
static fd_set active_fd_set; /* FD sets of all active sockets. */
static fd_set read_fd_set;   /* FD sets of all sockets read to be read. */
static int max_fd = 4; /* Largest used FD so far. */
static SSL_CTX* ssl_ctx; /* SSL context for this proxy. */

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
 * @brief Initialize SSL library.
 */
void init_ssl(void)
{
    /* Register libcrypto and libssl error strings. */
    SSL_load_error_strings();

    /* Register available SSL/TLS ciphers and digests. */
    SSL_library_init();

    /* Add all digest and cipher algoritms to the table. */
    OpenSSL_add_all_algorithms();

    /* Create a new SSL_CTX object as framework for TLS/SSL functions. */
    ssl_ctx = SSL_CTX_new(SSLv23_method());
    if (ssl_ctx == NULL) {
        ERR_print_errors_fp(stderr);
        LOG_FATAL("SSL_CTX_new");
    }

    /*  Load certificates. */
    if (SSL_CTX_use_certificate_file(ssl_ctx,
                                     CERT_FILE,
                                     SSL_FILETYPE_PEM) != 1) {
        ERR_print_errors_fp(stderr);
        LOG_FATAL("SSL_CTX_use_certificate_file");
    }

    /* Load private key and implicitly check the consistency of the private key
     * with the certificate. */
    if (SSL_CTX_use_PrivateKey_file(ssl_ctx,
                                    KEY_FILE,
                                    SSL_FILETYPE_PEM) != 1) {
        ERR_print_errors_fp(stderr);
        LOG_FATAL("SSL_CTX_use_PrivateKey_file");
    }
}

/**
 * @brief Clear data structure for SSL library.
 */
void clear_ssl(void)
{
    /* SSL context for this proxy. */
    SSL_CTX_free(ssl_ctx);

    /* Free SSL_COMP_get_compression_methods() called by SSL_library_init(). */
    sk_SSL_COMP_free(SSL_COMP_get_compression_methods());

    /* Remove all cipher and digest algorithms from the table. */
    EVP_cleanup();

    CRYPTO_cleanup_all_ex_data();

    /* Free the error queue associated with the current thread. */
    ERR_remove_thread_state(NULL);

    /* Free all loaded error strings. */
    ERR_free_strings();
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

    init_ssl();

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

    clear_ssl();
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
    bcopy((char *)server->h_addr,
          (char *)&server_addr.sin_addr.s_addr,
          server->h_length);
    server_addr.sin_port = htons(port);

    /* Create a connection with the server. */
    if (connect(server_sock,
                (struct sockaddr *)&server_addr,
                sizeof(server_addr)) < 0) {
        PLOG_ERROR("connect");
        return -1;
    }

    /* Create socket buffer for this server. */
    if (sock_buf_add_server(server_sock,
                            client_sock,
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
    struct sock_buf* client_buf = NULL;
    struct sock_buf* server_buf = NULL;

    /* Close SSL connection. */
    server_buf = sock_buf_get(fd);
    if (server_buf->ssl) {
        SSL_shutdown(server_buf->ssl);
        SSL_free(server_buf->ssl);

        /* Close SSL connection of the corresponding client. */
        client_buf = sock_buf_get(server_buf->peer);
        SSL_shutdown(client_buf->ssl);
        SSL_free(client_buf->ssl);
    }
    /* Close TCP connection. */
    close(fd);
    /* Remove from FD set for select(). */
    FD_CLR(fd, &active_fd_set);
    /* Remove socket buffer. */
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
    struct sock_buf* client_buf = NULL;
    struct sock_buf* server_buf = NULL;

    /* Close SSL connection. */
    client_buf = sock_buf_get(fd);
    if (client_buf->ssl) {
        SSL_shutdown(client_buf->ssl);
        SSL_free(client_buf->ssl);
    }
    /* Close TCP connection. */
    close(fd);
    /* Remove from FD set for select(). */
    FD_CLR(fd, &active_fd_set);
    /* Remove socket buffer. */
    sock_buf_rm(fd);

    /* Remove socket buffer of its requested servers. */
    for (int i = 0; i < FD_SETSIZE; ++i) {
        server_buf = sock_buf_get(i);
        if (server_buf != NULL && server_buf->peer == fd) {
            disconnect_server(i);
        }
    }

    LOG_INFO("disconnect client (fd: %d)", fd);
}

/**
 * Establish SSL connection to server.
 *
 * @param hostname Server hostname without port number.
 * @param port Server port number.
 * @param client_sock FD for client socket.
 * @return Socket of the new connected server.
 */
void ssl_connect_server(const char* hostname, int port, int fd)
{
    int server_sock;
    struct sock_buf* sock_buf = NULL;
    SSL* ssl = NULL;
    int ret = 0;

    server_sock = connect_server(hostname, port, fd, NULL);
    sock_buf = sock_buf_get(server_sock);
    ssl = SSL_new(ssl_ctx);
    if (ssl == NULL) {
        PLOG_ERROR("SSL_new");
        disconnect_server(server_sock);
        return;
    }
    sock_buf->ssl = ssl;
    ret = SSL_set_fd(ssl, server_sock);
    if (ret == 0) {
        PLOG_ERROR("SSL_set_fd");
        disconnect_server(server_sock);
        return;
    }
    ret = SSL_connect(ssl);
    if (ret != 1) {
        PLOG_ERROR("SSL_connect");
        SSL_get_error(ssl, ret);
        disconnect_server(server_sock);
        return;
    }
}

/**
 * @brief Establish SSL connection with client.
 * 
 * @param fd FD for client socket.
 */
void ssl_accept_client(int fd)
{
    struct sock_buf* sock_buf = NULL;
    SSL* ssl = NULL;
    int ret = 0;

    sock_buf = sock_buf_get(fd);
    ssl = SSL_new(ssl_ctx);
    if (ssl == NULL) {
        PLOG_ERROR("SSL_new");
        disconnect_client(fd);
        return;
    }
    sock_buf->ssl = ssl;
    ret = SSL_set_fd(ssl, fd);
    if (ret == 0) {
        PLOG_ERROR("SSL_set_fd");
        disconnect_client(fd);
        return;
    }
    ret = SSL_accept(ssl);
    if (ret != 1) {
        LOG_ERROR("SSL_connect");
        ERR_print_errors_fp(stderr);
        disconnect_client(fd);
        return;
    }
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
    else if(n < size){
        LOG_ERROR("Cannot write the whole message");
        return -2;
    }
    else {
        LOG_INFO("replied Connection Established");
    }

    free(message);
    message = NULL;

    return 0;
}

/**
 * @brief Handle GET request.
 * 
 * @param fd FD for client socket.
 * @param request Client request.
 * @param request_len Byte size of client request.
 * @param url URL in client request.
 * @param hostname Hostname in client request.
 * @param port Port number in client request.
 */
void handle_get_request(int fd,
                        char* request,
                        int request_len,
                        char* url,
                        char* hostname,
                        int port) {
    char* key = NULL;
    char* val = NULL;
    int val_len = 0;
    int age = 0;
    int n;
    int server_sock;

    /* Check cache. */
    /* Use hostname + url as cache key. */
    key = malloc(strlen(hostname) + strlen(url) + 1);
    if (key == NULL) {
        PLOG_FATAL("malloc");
    }
    strcpy(key, hostname);
    strcat(key, url);
    if (cache_get(key, &val, &val_len, &age) > 0) {
        char* head = NULL;
        int head_len = 0;
        char* body = NULL;
        int body_len = 0;
        char* age_line = NULL;

        LOG_INFO("cache hit");

        /* Create header line for age field. */
        age_line = malloc(100);
        if (age_line == NULL) {
            PLOG_ERROR("malloc");
        }
        else {
            bzero(age_line, 100);
            sprintf(age_line, "Age: %d\r\n", age);
        }

        /* Forward cached response to the client. */
        parse_body_head(val, val_len, &head, &head_len, &body, &body_len);
        n = write(fd, head, head_len);
        if (age_line != NULL) {
            n = write(fd, age_line, strlen(age_line));
        }
        n = write(fd, "\r\n", strlen("\r\n"));
        n = write(fd, body, body_len);
        if (n < 0) {
            PLOG_ERROR("write");
            disconnect_client(fd);
        }
        else if (n == 0) {
            LOG_ERROR("client socket is closed on the other side");
            disconnect_client(fd);
        }
        else {
          LOG_INFO("forward %d bytes from cache to client (fd %d)",
                   val_len,
                   fd);
        }

        free(key);
        key = NULL;
        free(val);
        val = NULL;
        free(head);
        head = NULL;
        free(body);
        body = NULL;
        free(age_line);
        age_line = NULL;

        return;
    }
    LOG_INFO("cache miss");

    /* Connect the requested server. */
    server_sock = connect_server(hostname, port, fd, key);
    if (server_sock < 0) {
        /* Fail to connect the request server. */
        free(key);
        key = NULL;
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

    free(key);
    key = NULL;
}

/**
 * @brief Handle a CONNECT request.
 * 
 * @param fd FD for client socket.
 * @param version String of HTTP version field in the request.
 * @param hostname Hostname to request.
 * @param port Port number to request.
 */
void handle_connect_request(int fd, char* version, char* hostname, int port)
{
    /* Establish SSL connection with server. */
    ssl_connect_server(hostname, port, fd);
    LOG_INFO("established SSL connection with %s:%d", hostname, port);

    reply_connection_established(fd, version);

    /* Establish SSL connection with client. */
    ssl_accept_client(fd);
    LOG_INFO("established SSL connection with client (fd %d)", fd);
}

/**
 * @brief Handle client request if the request inf buffer is completed.
 * 
 * @param fd FD for client socket.
 */
void handle_client_request(int fd)
{
    struct sock_buf* sock_buf = sock_buf_get(fd);
    char* request = NULL;
    int request_len = 0;
    char* method = NULL; /* Method field in client request. */
    char* url = NULL; /* URL field in client request. */
    char* version = NULL; /* Version field in client request. */
    char* host = NULL; /* Host field in client request. */
    char* hostname = NULL; /* Server hostname without port number. */
    int port = -1; /* Server port in client request. 80 by default. */

    if (sock_buf == NULL) {
        return;
    }

    /* Extract the leading completed request. */
    while (extract_first_request(&(sock_buf->buf),
                                 &(sock_buf->size),
                                 &request,
                                 &request_len) > 0) {
        /* Parse request. */
        parse_request_head(request, &method, &url, &version, &host);
        parse_host_field(host, &hostname, &port);
        LOG_INFO("parsed request:\n"
                 "- method: %s\n"
                 "- url: %s\n"
                 "- version: %s\n"
                 "- host: %s\n"
                 "- hostname: %s",
                 method,
                 url,
                 version,
                 host,
                 hostname);

        if (strcmp(method, "GET") == 0) {
            LOG_INFO("handle GET method");

            if (port < 0) {
                port = 80; /* Default port for HTTP. */
            }
            LOG_INFO("port: %d", port);

            handle_get_request(fd, request, request_len, url, hostname, port);
        }
        else if (strcmp(method, "CONNECT") == 0) {
            LOG_INFO("handle CONNECT method");

            if (port < 0) {
                port = 443; /* Default port for SSL link. */
            }
            LOG_INFO("port: %d", port);

            handle_connect_request(fd, version, hostname, port);
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
        free(request);
        request = NULL;
    }
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
    if (extract_first_response(&(sock_buf->buf),
                                &(sock_buf->size),
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
    n = write(sock_buf->peer, response, response_len);
    if (n < 0) {
        PLOG_ERROR("write");
        disconnect_client(sock_buf->peer);
    }
    else if (n == 0) {
        LOG_ERROR("client socket is closed on the other side");
        disconnect_client(sock_buf->peer);
    }
    else {
        LOG_INFO("forward %d bytes from server (fd %d) to client (fd %d)",
                response_len,
                fd,
                sock_buf->peer);
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

    sock_buf = sock_buf_get(fd);
    if (sock_buf == NULL) {
        LOG_ERROR("unknown socket %d", fd);
        return;
    }
    is_client = sock_buf_is_client(fd);

    /* Receive message. */
    bzero(buf, BUF_SIZE);
    n = read(fd, buf, BUF_SIZE);
    if (n < 0) {
        PLOG_ERROR("read");
        if (is_client) {
            disconnect_client(fd);
            return;
        }
        else {
            disconnect_server(fd);
            return;
        }
    }
    else if (n == 0) {
        /* Socket is disconnected on the other side. */
        if (is_client) {
            LOG_INFO("client socket is closed on the other side");
            disconnect_client(fd);
            return;
        }
        else {
            LOG_INFO("server socket is closed on the other side");
            disconnect_server(fd);
            return;
        }
    }
    #if 1
    if (is_client) {
        LOG_INFO("received %d bytes from client (fd: %d)", n, fd);
    }
    else {
        LOG_INFO("received %d bytes from server (fd: %d)", n, fd);
    }
    #endif

    /* Write received message into socket buffer. */
    if (sock_buf_buffer(fd, buf, n) < 0) {
        PLOG_ERROR("sock_buf_input");
        return;
    }

    /* Parse socket buffer. */
    if (is_client) {
         handle_client_request(fd);
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

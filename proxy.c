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
#define CACHE_SIZE 100

static int listen_port; /* Port that proxy listens on. */
static int listen_sock; /* Listening socket of the proxy. */
static fd_set active_fd_set; /* FD sets of all active sockets. */
static fd_set read_fd_set;   /* FD sets of all sockets read to be read. */
static int max_fd = 4; /* Largest used FD so far. */
static SSL_CTX* ssl_ctx; /* SSL context for this proxy. */
static int use_ssl = 0; /* Whether to use SSL interception. */
static const char* CERT_FILE = NULL; /* Certificate file for SSL. */
static const char* KEY_FILE = NULL; /* Private key file for SSL. */

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

    if (use_ssl) {
        init_ssl();
    }

    /* Init FD set for select(). */
    FD_ZERO(&active_fd_set);
    FD_SET(listen_sock, &active_fd_set);

    /* Init LRU cache. */
    cache_init(CACHE_SIZE);

    /* Init socket buffer array. */
    sock_buf_arr_init();
}

/**
 * @brief Free all the proxy resource.
 */
void clear_proxy(void)
{
    /* Free LRU cache. */
    cache_clear();

    /* Free socket buffer array. */
    sock_buf_arr_clear();

    /* Close all sockets. */
    for (int fd = 0; fd < FD_SETSIZE; ++fd) {
        if (FD_ISSET(fd, &active_fd_set)) {
            FD_CLR(fd, &active_fd_set);
            close(fd);
        }
    }

    if (use_ssl) {
        clear_ssl();
    }
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
    int is_forward = 0;
    int peer;

    if (sock_buf_get(fd) == NULL) {
        return;
    }

    /* Close SSL connection. */
    if (sock_buf_is_ssl(fd)) {
        /* Close SSL connection between the proxy and the server.*/
        server_buf = sock_buf_get(fd);
        SSL_shutdown(server_buf->ssl);
        SSL_free(server_buf->ssl);
        server_buf->ssl = NULL;

        /* Close SSL connection between the proxy and the client.*/
        client_buf = sock_buf_get(server_buf->peer);
        if (client_buf != NULL && client_buf->ssl != NULL) {
            SSL_shutdown(client_buf->ssl);
            SSL_free(client_buf->ssl);
            client_buf->ssl = NULL;
        }
    }

    /* Close TCP connection. */
    close(fd);

    /* Remove from FD set for select(). */
    FD_CLR(fd, &active_fd_set);

    /* Find the peer that directly forward to. */
    is_forward = sock_buf_is_forward(fd);
    if (is_forward) {
        /* Disconnect client that directly forward to. */
        server_buf = sock_buf_get(fd);
        peer = server_buf->peer;
    }

    /* Remove socket buffer. */
    sock_buf_rm(fd);

    /* Disconnect the peer that directly forward to. */
    if (is_forward) {
        disconnect_client(peer);
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
    struct sock_buf* server_buf = NULL;

    if (sock_buf_get(fd) == NULL) {
        return;
    }

    /* Close TCP connection. */
    close(fd);

    /* Remove from FD set for select(). */
    FD_CLR(fd, &active_fd_set);

    /* Remove socket buffer. */
    sock_buf_rm(fd);

    /* Close connected server. */
    for (int i = 0; i <= max_fd; ++i) {
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
 * @return int Socket of the new connected server on success; -1 otherwise.
 */
int ssl_connect_server(const char* hostname, int port, int client_sock)
{
    int server_sock;
    struct sock_buf* sock_buf = NULL;
    SSL* ssl = NULL;

    server_sock = connect_server(hostname, port, client_sock, NULL);
    if (server_sock < 0) {
        /* Fail to connect to the server. */
        return -1;
    }
    sock_buf = sock_buf_get(server_sock);
    if (sock_buf == NULL) {
        LOG_ERROR("unknown socket %d", client_sock);
        return -1;
    }
    ssl = SSL_new(ssl_ctx);
    if (ssl == NULL) {
        LOG_ERROR("SSL_new");
        ERR_print_errors_fp(stderr);
        disconnect_server(server_sock);
        return -1;
    }
    if (SSL_set_fd(ssl, server_sock) == 0) {
        LOG_ERROR("SSL_set_fd");
        ERR_print_errors_fp(stderr);
        disconnect_server(server_sock);
        return -1;
    }
    if (SSL_connect(ssl) != 1) {
        LOG_ERROR("SSL_connect");
        ERR_print_errors_fp(stderr);
        disconnect_server(server_sock);
        return -1;
    }
    sock_buf->ssl = ssl;
    sock_buf->peer = client_sock;
    return server_sock;
}

/**
 * @brief Establish SSL connection with client.
 * 
 * @param client_sock FD for client socket.
 * @param server_sock FD for client socket.
 * @return int 0 on success; -1 otherwise.
 */
int ssl_accept_client(int client_sock, int server_sock)
{
    struct sock_buf* sock_buf = NULL;
    SSL* ssl = NULL;

    sock_buf = sock_buf_get(client_sock);
    if (sock_buf == NULL) {
        LOG_ERROR("unknown socket %d", client_sock);
        return -1;
    }
    ssl = SSL_new(ssl_ctx);
    if (ssl == NULL) {
        LOG_ERROR("SSL_new");
        ERR_print_errors_fp(stderr);
        disconnect_client(client_sock);
        return -1;
    }
    if (SSL_set_fd(ssl, client_sock) == 0) {
        LOG_ERROR("SSL_set_fd");
        ERR_print_errors_fp(stderr);
        disconnect_client(client_sock);
        return -1;
    }
    if (SSL_accept(ssl) != 1) {
        LOG_ERROR("SSL_accept");
        ERR_print_errors_fp(stderr);
        disconnect_client(client_sock);
        return -1;
    }
    sock_buf->ssl = ssl;
    sock_buf->peer = server_sock;
    return 0;
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
    struct sock_buf* client_buf = NULL;
    struct sock_buf* server_buf = NULL;
    int is_ssl = 0;
    char* key = NULL;
    char* val = NULL;
    int val_len = 0;
    int age = 0;
    int n;
    int server_sock;

    client_buf = sock_buf_get(fd);
    if (client_buf == NULL) {
        LOG_ERROR("unknown socket %d", fd);
        return;
    }
    is_ssl = sock_buf_is_ssl(fd);

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
        if (is_ssl) {
            n = SSL_write(client_buf->ssl, head, head_len);
            if (age_line != NULL) {
                n = SSL_write(client_buf->ssl, age_line, strlen(age_line));
            }
            n = SSL_write(client_buf->ssl, "\r\n", strlen("\r\n"));
            n = SSL_write(client_buf->ssl, body, body_len);
        }
        else {
            n = write(fd, head, head_len);
            if (age_line != NULL) {
                n = write(fd, age_line, strlen(age_line));
            }
            n = write(fd, "\r\n", strlen("\r\n"));
            n = write(fd, body, body_len);
        }
        if (n < 0) {
            if (is_ssl) {
                ERR_print_errors_fp(stderr);
                LOG_ERROR("SSL_write");
            }
            else {
                PLOG_ERROR("write");
            }
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
    if (is_ssl) {
        server_sock = client_buf->peer;
        server_buf = sock_buf_get(server_sock);
        if (server_buf == NULL) {
            LOG_ERROR("unknown socket %d", fd);
            free(key);
            key = NULL;
            return;
        }
        server_buf->key = strdup(key);
    }
    else {
        server_sock = connect_server(hostname, port, fd, key);
        if (server_sock < 0) {
            /* Fail to connect the request server. */
            free(key);
            key = NULL;
            return;
        }
    }

    /* Forward request to server. */
    if (is_ssl) {
        n = SSL_write(server_buf->ssl, request, request_len);
    }
    else {
        n = write(server_sock, request, request_len);
    }
    if (n < 0) {
        if (is_ssl) {
            ERR_print_errors_fp(stderr);
            LOG_ERROR("SSL_write");
        }
        else {
            PLOG_ERROR("write");
        }
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
 * @param client_sock FD for client socket.
 * @param version String of HTTP version field in the request.
 * @param hostname Hostname to request.
 * @param port Port number to request.
 */
void handle_connect_request(int client_sock,
                           char* version,
                           char* hostname,
                           int port)
{
    int server_sock;

    if (use_ssl) {
        /* Establish SSL connection with server. */
        server_sock = ssl_connect_server(hostname, port, client_sock);
        if (server_sock < 0) {
            LOG_ERROR("ssl_connect_server");
            return;
        }
        LOG_INFO("established SSL connection with %s:%d", hostname, port);

        reply_connection_established(client_sock, version);

        /* Establish SSL connection with client. */
        if (ssl_accept_client(client_sock, server_sock) < 0) {
            LOG_ERROR("ssl_accept_client");
            return;
        }
        LOG_INFO("established SSL connection with client (fd %d)", client_sock);
    }
    else {
        struct sock_buf* client_buf = NULL;
        struct sock_buf* server_buf = NULL;

        /* Connect server. */
        server_sock = connect_server(hostname, port, client_sock, NULL);
        if (server_sock < 0) {
            return;
        }

        /* Setup 2-way forwarding. */
        client_buf = sock_buf_get(client_sock);
        server_buf = sock_buf_get(server_sock);
        client_buf->peer = server_sock;
        client_buf->is_forward = 1;
        server_buf->peer = client_sock;
        server_buf->is_forward = 1;

        /* Reply client with "Connection Established". */
        reply_connection_established(client_sock, version);
    }
}

/**
 * @brief Handle other request by directly forwarding it to server.
 * 
 * @param fd FD for client socket.
 * @param request Client request.
 * @param request_len Byte size of client request.
 * @param hostname Hostname in client request.
 * @param port Port number in client request.
 */
void handle_other_request(int fd,
                          char* request,
                          int request_len,
                          char* hostname,
                          int port) {
    struct sock_buf* client_buf = NULL;
    struct sock_buf* server_buf = NULL;
    int is_ssl = 0;
    int n;
    int server_sock;

    client_buf = sock_buf_get(fd);
    if (client_buf == NULL) {
        LOG_ERROR("unknown socket %d", fd);
        return;
    }
    is_ssl = sock_buf_is_ssl(fd);

    /* Connect the requested server. */
    if (is_ssl) {
        server_sock = client_buf->peer;
        server_buf = sock_buf_get(server_sock);
        if (server_buf == NULL) {
            LOG_ERROR("unknown socket %d", fd);
            return;
        }
    }
    else {
        server_sock = connect_server(hostname, port, fd, NULL);
        if (server_sock < 0) {
            /* Fail to connect the request server. */
            return;
        }
    }

    /* Forward request to server. */
    if (is_ssl) {
        n = SSL_write(server_buf->ssl, request, request_len);
    }
    else {
        n = write(server_sock, request, request_len);
    }
    if (n < 0) {
        if (is_ssl) {
            ERR_print_errors_fp(stderr);
            LOG_ERROR("SSL_write");
        }
        else {
            PLOG_ERROR("write");
        }
        disconnect_server(server_sock);
    }
    if (n == 0) {
        LOG_ERROR("server socket is closed on the other side");
        disconnect_server(server_sock);
    }
}

/**
 * @brief Handle client request if the request inf buffer is completed.
 * 
 * @param fd FD for client socket.
 */
void handle_client_request(int fd)
{
    struct sock_buf* sock_buf = NULL;
    char* request = NULL;
    int request_len = 0;
    char* method = NULL; /* Method field in client request. */
    char* url = NULL; /* URL field in client request. */
    char* version = NULL; /* Version field in client request. */
    char* host = NULL; /* Host field in client request. */
    char* hostname = NULL; /* Server hostname without port number. */
    int port = -1; /* Server port in client request. 80 by default. */
    int is_ssl = 0; /* Whether the client is using SSL connection. */

    sock_buf = sock_buf_get(fd);
    if (sock_buf == NULL) {
        return;
    }
    is_ssl = sock_buf_is_ssl(fd);

    /* Extract the leading completed request. */
    while (extract_first_request(&(sock_buf->buf),
                                 &(sock_buf->size),
                                 &request,
                                 &request_len) > 0) {
        /* Parse request. */
        parse_request_head(request, &method, &url, &version, &host);
        port = -1;
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
                if (is_ssl) {
                    port = 443; /* Default port for SSL link. */
                }
                else {
                    port = 80; /* Default port for HTTP. */
                }
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
            handle_other_request(fd, request, request_len, hostname, port);
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
    struct sock_buf* server_buf = NULL;
    int n;
    int is_ssl = 0;

    server_buf = sock_buf_get(fd);
    if (server_buf == NULL) {
        LOG_ERROR("unknown socket %d", server_buf);
        return;
    }
    is_ssl = sock_buf_is_ssl(fd);

    /* Extract the leading completed response. */
    if (extract_first_response(&(server_buf->buf),
                                &(server_buf->size),
                                &response,
                                &response_len,
                                &max_age,
                                &(server_buf->is_chunked)) == 0) {
        /* Response is incomplete.*/
        return;
    }

    /* Cache response. */
    if (cache_put(server_buf->key, response, response_len, max_age) == 0) {
        LOG_ERROR("fail to cache server response");
    }

    /* Forward response to client. */
    if (is_ssl) {
        struct sock_buf* client_buf = NULL;

        client_buf = sock_buf_get(server_buf->peer);
        if (client_buf == NULL) {
            LOG_ERROR("unknown socket %d", client_buf);
            return;
        }
        if (!sock_buf_is_ssl(server_buf->peer)) {
            LOG_ERROR("client is not in SSL connection");
            return;
        }
        n = SSL_write(client_buf->ssl, response, response_len);
    }
    else {
        n = write(server_buf->peer, response, response_len);
    }
    if (n < 0) {
        if (is_ssl) {
            LOG_ERROR("SSL_write");
            ERR_print_errors_fp(stderr);
        }
        else {
            PLOG_ERROR("write");
        }
        disconnect_client(server_buf->peer);
    }
    else if (n == 0) {
        LOG_ERROR("client socket is closed on the other side");
        disconnect_client(server_buf->peer);
    }
    else {
        LOG_INFO("forward %d bytes from server (fd %d) to client (fd %d)",
                response_len,
                fd,
                server_buf->peer);
    }

    /* Disconnect server. */
    if (!is_ssl) {
        disconnect_server(fd);
    }

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
    int is_ssl = 0; /* Whether this socket is one end of a SSL connection. */
    int is_forward = 0; /* Whether simply forward data to its peer. */

    sock_buf = sock_buf_get(fd);
    if (sock_buf == NULL) {
        LOG_ERROR("unknown socket %d", fd);
        return;
    }
    is_client = sock_buf_is_client(fd);
    is_forward = sock_buf_is_forward(fd);
    is_ssl = sock_buf_is_ssl(fd);

    /* Receive message. */
    bzero(buf, BUF_SIZE);
    if (is_ssl) {
        n = SSL_read(sock_buf->ssl, buf, BUF_SIZE);
    }
    else {
        n = read(fd, buf, BUF_SIZE);
    }
    if (n < 0) {
        if (is_ssl) {
            ERR_print_errors_fp(stderr);
            LOG_ERROR("SSL_read");
        }
        else {
            PLOG_ERROR("read");
        }
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
    #if 0
    if (is_client) {
        LOG_INFO("received %d bytes from client (fd: %d): %s", n, fd, buf);
    }
    else {
        LOG_INFO("received %d bytes from server (fd: %d): %s", n, fd, buf);
    }
    #endif

    /* Update the last input time of the socket. */
    sock_buf_update_input_time(fd);

    /* Forward encrypted messages originated from a CONNECT method. */
    if (is_forward) {
        #if 0
        LOG_INFO("forwarding encrypted data from fd %d to fd %d",
                fd,
                sock_buf->peer);
        #endif
        n = write(sock_buf->peer, buf, n);
        if (n < 0) {
            PLOG_ERROR("write");
            if (is_client) {
                disconnect_server(sock_buf->peer);
            } else {
                disconnect_client(sock_buf->peer);
            }
        }
        if (n == 0) {
            LOG_INFO("CONNECT socket is closed on the other side");
            if (is_client) {
                disconnect_server(sock_buf->peer);
            }
            else {
                disconnect_client(sock_buf->peer);
            }
        }
        return;
    }

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
    if (argc != 2 && argc != 4) {
        fprintf(stderr, "usage: %s <port> [<cert_file> <key_file>]", argv[0]);
        exit(EXIT_FAILURE);
    }
    listen_port = atoi(argv[1]);
    if (argc == 4) {
        use_ssl = 1; /* Raise flag for SSL interception. */
        CERT_FILE = argv[2];
        KEY_FILE = argv[3];
        LOG_INFO("use SSL interception");
    }

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
            /*remove timeout socket*/
            if (sock_buf_is_timeout(fd)) {
                if (sock_buf_is_client(fd)) {
                    disconnect_client(fd);
                }
                else {
                    disconnect_server(fd);
                }
            }
        }
    }

    clear_proxy();
    LOG_INFO("shut down");

    return EXIT_SUCCESS;
}

/**************************************************************
*
*                        sock_buf.h
*
*     Final Project: High Performance HTTP Proxy
*     Author:  Keren Zhou (kzhou), Ruiyuan Gu (rgu03)
*     Date: 2021-11-11
*
*     Summary:
*     Interface for socket message buffer.
*
**************************************************************/

#ifndef SOCK_BUF_H
#define SOCK_BUF_H

#include <time.h>
#include <openssl/ssl.h>

struct sock_buf {
    char* buf; /* Buffer for plaintext received from the socket. */
    int size; /* Byte size of buffered data. */
    time_t last_input; /* Time for the last input to the buffer. */
    int is_client; /* Whether the socket is for a client. */
    SSL* ssl; /* SSL structure for SSL/TLS connection. */
    int peer; /* Socket FD for the other end of the connection regardless of
               * proxy. */
    char* key; /* Key for the cached server response. */
};

/**
 * @brief Create an empty socket message buffer array.
 *
 * @return int 0 if succeed; -1 otherwise.
 */
int sock_buf_arr_init(void);

/**
 * @brief Cleanup the socket message buffer array.
 *
 * @return int 0 if succeed; -1 otherwise.
 */
int sock_buf_arr_clear(void);

/**
 * @brief Add socket message buffer of the given FD.
 * 
 * @param fd FD for socket.
 * @return int Number of socket message buffer added, i.e. 1 if succeeds; 0
 * otherwise.
 * @return int Number of added client buffer, i.e. 1 on success; 0 otherwise.
 */
int sock_buf_add_client(int fd);

/**
 * @brief Add socket message buffer of the given FD.
 * 
 * @param fd FD for socket.
 * @return int Number of socket message buffer added, i.e. 1 if succeeds; 0
 * otherwise.
 * @param key String of cache key, i.e. hostname + url in GET request.
 * @return int Number of added server buffer, i.e. 1 on success; 0 otherwise.
 */
int sock_buf_add_server(int fd, int client, char* key);

/**
 * @brief Remove socket message buffer of the given FD.
 * 
 * @param fd FD for socket.
 * @return int Number of socket message buffer removed, i.e. 1 if succeeds; 0
 * otherwise.
 */
int sock_buf_rm(int fd);

/**
 * @brief Get socket message buffer of the given FD.
 * 
 * @param fd FD for socket.
 * @return struct sock_buf* Pointer to socket message buffer if succeeds; NULL
 * otherwise.
 */
struct sock_buf* sock_buf_get(int fd);

/**
 * @brief Whether the given socket is for a client.
 *
 * @param fd FD for socket.
 * @return int 1 if the socket is for a client; 0 otherwise (typically, the
 * socket is for a server).
 */
int sock_buf_is_client(int fd);

/**
 * @brief Whether the given socket is one end of a SSL/TLS connection.
 *
 * @param fd FD for socket.
 * @return int 1 if the socket is one end of a SSL/TLS connection.
 */
int sock_buf_is_ssl(int fd);

/**
 * @brief Buffer the received data.
 *
 * @param fd FD for socket.
 * @param data  Received data.
 * @param size Byte size of received data.
 * @return int Byte size of buffered data on success; -1 otherwise.
 */
int sock_buf_buffer(int fd, char* data, int size);

#endif /* SOCK_BUF_H */

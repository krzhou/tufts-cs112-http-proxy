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

struct sock_buf {
    char* msg;
    int len;
    int client; /* FD for client socket; -1 for client. */
    time_t last_access;
    int connected_sock;
    char* key; /* String of key, i.e. hostname + url from the GET request. */
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
 * @param is_connect Whether this server socket is for a CONNECT request.
 * @param key String of cache key, i.e. hostname + url in GET request.
 * @return int Number of added server buffer, i.e. 1 on success; 0 otherwise.
 */
int sock_buf_add_server(int fd, int client, int is_connect, char* key);

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

#endif /* SOCK_BUF_H */

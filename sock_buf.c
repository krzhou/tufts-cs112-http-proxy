/**************************************************************
*
*                        sock_buf.c
*
*     Final Project: High Performance HTTP Proxy
*     Author:  Keren Zhou (kzhou), Ruiyuan Gu (rgu03)
*     Date: 2021-11-11
*
*     Summary:
*     Implementation for socket message buffer.
*
**************************************************************/

#include "sock_buf.h"
#include "logger.h"
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>

static struct sock_buf *sock_buf_arr[FD_SETSIZE];

/**
 * @brief Create an empty socket message buffer array.
 *
 * @return int 0 if succeed; -1 otherwise.
 */
int sock_buf_arr_init(void)
{
    for (int i = 0; i < FD_SETSIZE; ++i) {
        sock_buf_arr[i] = NULL;
    }
    return 0;
}

/**
 * @brief Cleanup the socket message buffer array.
 *
 * @return int 0 if succeed; -1 otherwise.
 */
int sock_buf_arr_clear(void)
{
    for (int i = 0; i < FD_SETSIZE; ++i) {
        sock_buf_rm(i);
    }
    return 0;
}

/**
 * @brief Check whether FD is valid, i.e. 0 <= FD < FD_SETSIZE.
 *
 * @param fd FD to check.
 * @return int 1 if valid; 0 otherwise.
 */
int is_valid_fd(int fd) {
    return 0 <= fd && fd < FD_SETSIZE;
}

/**
 * @brief Add socket message buffer of the given FD.
 * 
 * @param fd FD for socket.
 * @return int Number of socket message buffer added, i.e. 1 if succeeds; 0
 * otherwise.
 * @return int Number of added client buffer, i.e. 1 on success; 0 otherwise.
 */
int sock_buf_add_client(int fd)
{
    struct sock_buf* new_sock_buf = NULL;

    if (!is_valid_fd(fd) || sock_buf_arr[fd] != NULL) {
        return 0;
    }

    new_sock_buf = (struct sock_buf*)malloc(sizeof(struct sock_buf));
    if (new_sock_buf == NULL) {
        PLOG_ERROR("malloc");
        return 0;
    }
    new_sock_buf->msg = NULL;
    new_sock_buf->len = 0;
    new_sock_buf->client = -1;
    new_sock_buf->last_access = 0;
    new_sock_buf->connected_sock = -1;
    new_sock_buf->key = NULL;
    sock_buf_arr[fd] = new_sock_buf;
    return 1;
}

/**
 * @brief Add socket message buffer of the given FD.
 * 
 * @param fd FD for socket.
 * @return int Number of socket message buffer added, i.e. 1 if succeeds; 0
 * otherwise.
 * @param connected_sock Socket for the other side of a CONNECT method; -1 if 
 * method is not CONNECT.
 * @param key String of cache key, i.e. hostname + url in GET request.
 * @return int Number of added server buffer, i.e. 1 on success; 0 otherwise.
 */
int sock_buf_add_server(int fd, int client, int connected_sock, char* key)
{
    struct sock_buf* new_sock_buf = NULL;

    if(!is_valid_fd(fd) ||
       !is_valid_fd(client) ||
       sock_buf_arr[fd] != NULL ||
       sock_buf_arr[client] == NULL) {
        return 0;
    }

    new_sock_buf = (struct sock_buf*) malloc(sizeof(struct sock_buf));
    if (new_sock_buf == NULL) {
        PLOG_ERROR("malloc");
        return 0;
    }
    new_sock_buf->msg = NULL;
    new_sock_buf->len = 0;
    new_sock_buf->client = client;
    new_sock_buf->last_access = 0;
    new_sock_buf->connected_sock = connected_sock;
    new_sock_buf->key = NULL;
    if (key != NULL) {
        new_sock_buf->key = strdup(key);
    }
    sock_buf_arr[fd] = new_sock_buf;
    return 1;
}

/**
 * @brief Remove socket message buffer of the given FD.
 * 
 * @param fd FD for socket.
 * @return int Number of socket message buffer removed, i.e. 1 if succeeds; 0
 * otherwise.
 */
int sock_buf_rm(int fd)
{
    if(!is_valid_fd(fd) || sock_buf_arr[fd] == NULL) {
        return 0;
    }

    free(sock_buf_arr[fd]->msg);
    free(sock_buf_arr[fd]->key);
    free(sock_buf_arr[fd]);
    sock_buf_arr[fd] = NULL;
    return 1;
}

/**
 * @brief Get socket message buffer of the given FD.
 * 
 * @param fd FD for socket.
 * @return struct sock_buf* Pointer to socket message buffer if succeeds; NULL
 * otherwise.
 */
struct sock_buf* sock_buf_get(int fd)
{
    if (!is_valid_fd(fd)) {
        return NULL;
    }
    return sock_buf_arr[fd];
}
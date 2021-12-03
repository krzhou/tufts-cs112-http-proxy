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
 * @return int 0 on success; -1 otherwise.
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
 * @return int 0 on success; -1 otherwise.
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
 * @brief Add socket message buffer for a client.
 * 
 * @param fd FD for socket.
 * @return int Number of socket buffer added, i.e. 1 on success; 0; otherwise.
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
    new_sock_buf->buf = NULL;
    new_sock_buf->size = 0;
    new_sock_buf->last_input = 0;
    new_sock_buf->is_client = 1;
    new_sock_buf->ssl = NULL;
    new_sock_buf->peer = -1;
    new_sock_buf->key = NULL;
    new_sock_buf->is_chunked = 0;
    sock_buf_arr[fd] = new_sock_buf;
    return 1;
}

/**
 * @brief Add socket message buffer of the given FD.
 * 
 * @param fd FD for socket.
 * @param key String of cache key, i.e. hostname + url in GET request.
 * @return int Number of socket buffer added, i.e. 1 on success; 0 otherwise.
 */
int sock_buf_add_server(int fd, int client, char* key)
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
    new_sock_buf->buf = NULL;
    new_sock_buf->size = 0;
    new_sock_buf->last_input = 0;
    new_sock_buf->is_client = 0;
    new_sock_buf->ssl = NULL;
    new_sock_buf->peer = client;
    new_sock_buf->key = NULL;
    if (key != NULL) {
        new_sock_buf->key = strdup(key);
    }
    new_sock_buf->is_chunked = 0;
    sock_buf_arr[fd] = new_sock_buf;
    return 1;
}

/**
 * @brief Remove socket message buffer of the given FD.
 * 
 * @param fd FD for socket.
 * @return int Number of socket buffer removed, i.e. 1 on success; 0 otherwise.
 */
int sock_buf_rm(int fd)
{
    if(!is_valid_fd(fd) || sock_buf_arr[fd] == NULL) {
        return 0;
    }

    free(sock_buf_arr[fd]->buf);
    free(sock_buf_arr[fd]->key);
    if (sock_buf_arr[fd]->ssl != NULL) {
        SSL_shutdown(sock_buf_arr[fd]->ssl);
        SSL_free(sock_buf_arr[fd]->ssl);
        sock_buf_arr[fd] = NULL;
    }
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

/**
 * @brief Whether the given socket is for a client.
 *
 * @param fd FD for socket.
 * @return int 1 if the socket is for a client; 0 otherwise (typically, the
 * socket is for a server).
 */
int sock_buf_is_client(int fd)
{
    if (!is_valid_fd(fd) || sock_buf_arr[fd] == NULL) {
        return 0;
    }
    return sock_buf_arr[fd]->is_client;
}

/**
 * @brief Whether the given socket is one end of a SSL/TLS connection.
 *
 * @param fd FD for socket.
 * @return int 1 if the socket is one end of a SSL/TLS connection.
 */
int sock_buf_is_ssl(int fd)
{
    if (!is_valid_fd(fd) || sock_buf_arr[fd] == NULL) {
        return 0;
    }
    return sock_buf_arr[fd]->ssl != NULL;
}

/**
 * @brief Buffer the received data.
 *
 * @param fd FD for socket.
 * @param data  Received data.
 * @param size Byte size of received data.
 * @return int Byte size of buffered data on success; -1 otherwise.
 */
int sock_buf_buffer(int fd, char* data, int size)
{
    char* ret = NULL;

    if (!is_valid_fd(fd) || sock_buf_arr[fd] == NULL) {
        return -1;
    }

    ret = realloc(sock_buf_arr[fd]->buf, sock_buf_arr[fd]->size + size);
    if (ret == NULL) {
        return -1;
    }
    sock_buf_arr[fd]->buf = ret;
    memcpy(sock_buf_arr[fd]->buf + sock_buf_arr[fd]->size, data, size);
    sock_buf_arr[fd]->size += size;
    return size;
}

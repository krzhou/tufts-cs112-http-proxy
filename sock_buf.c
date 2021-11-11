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
#include <sys/select.h>

static struct sock_buf* sock_buf_arr[FD_SETSIZE];

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
    /* TODO */
    return 0;
}

/**
 * @brief Cleanup the socket message buffer array.
 *
 * @return int 0 if succeed; -1 otherwise.
 */
int sock_buf_arr_clear(void)
{
    /* TODO */
    return 0;
}

/**
 * @brief Add socket message buffer of the given FD.
 * 
 * @param fd FD for socket.
 * @return int Number of socket message buffer added, i.e. 1 if succeeds; 0
 * otherwise.
 */
int sock_buf_add(int fd)
{
    /* TODO */
    (void)fd;
    return 0;
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
    /* TODO */
    (void)fd;
    return 0;
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
    /* TODO */
    (void)fd;
    return 0;
}
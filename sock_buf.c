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
#include <string.h>

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
 * @brief Add socket message buffer of the given FD.
 * 
 * @param fd FD for socket.
 * @return int Number of socket message buffer added, i.e. 1 if succeeds; 0
 * otherwise.
 */
int sock_buf_add_client(int fd)
{
    /* TODO */
    if(sock_buf_arr[fd]!=NULL){
        return 0;
    }
    else{
        sock_buf_arr[fd]=(struct sock_buf*) malloc(sizeof(struct sock_buf));
        bzero(sock_buf_arr[fd],sizeof(struct sock_buf));
        
        return 1;
    }
    
}

/**
 * @brief Add socket message buffer of the given FD.
 * 
 * @param fd FD for socket.
 * @return int Number of socket message buffer added, i.e. 1 if succeeds; 0
 * otherwise.
 */
int sock_buf_add_server(int fd,int client)
{
    /* TODO */
    if(sock_buf_arr[fd]!=NULL||sock_buf_arr[client]==NULL){
        return 0;
    }
    else{
        sock_buf_arr[fd]=(struct sock_buf*) malloc(sizeof(struct sock_buf));
        bzero(sock_buf_arr[fd],sizeof(struct sock_buf));
        sock_buf_arr[fd]->client = client;
        return 1;
    }
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
    if(sock_buf_arr[fd]==NULL){
        return 0;
    }
    else{
        free(sock_buf_arr[fd]->msg);
        free(sock_buf_arr[fd]);
        sock_buf_arr[fd]=NULL;
        return 1;
    }
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
    return sock_buf_arr[fd];
}
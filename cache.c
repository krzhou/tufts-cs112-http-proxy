/**************************************************************
*
*                         cache.c
*
*     Final Project: High Performance HTTP Proxy
*     Author:  Keren Zhou (kzhou), Ruiyuan Gu (rgu03)
*     Date: 2021-11-11
*
*     Summary:
*     Implementation for fixed size LRU cache.
*
**************************************************************/

#include "cache.h"
#include <time.h>

static int size;
static int capacity;

struct cache_elem {
    char* key; /* String of hostname + url. */
    char* val; /* Full GET response. */
    int len; /* val may have '\0' in the middle. We need its byte size. */
    time_t create_time; /* Creation time in seconds. */
    time_t max_age; /* Time-to-live in seconds. */
    struct cache_elem* next;
    struct cache_elem* prev;
};

/* Doubly linked list of cache elements. */
static struct cache_elem* front;
static struct cache_elem* back;

/**
 * @brief Initialize an empty cache of the given capacity.
 *
 * @param capacity Max size of the cache, > 0.
 * @return 0 on success; -1 otherwise.
 */
int cache_init(int capacity)
{
    /* TODO */
    (void)capacity;
    return 0;
}

/**
 * Free the cache.
 *
 * @return 0 on success; -1 otherwise.
 */
int cache_clear(void);

/**
 * Put the given element (key, val, ttl) into cache.
 *
 * @param key Key of the element to be put, non-null.
 * @param val Value of the element to be put, non-null.
 * @param len Byte size of val.
 * @param max_age Time-to-live of the element in seconds, >= 0.
 * @return Number of elements put into cache.
 */
int cache_put(const char* key,
              const char* val,
              const int len,
              const int max_age)
{
    /* TODO */
    (void)key;
    (void)val;
    (void)len;
    (void)max_age;
    return 0;
}

/**
 * Get value of key from cache.
 *
 * If the key is found, *out_val will be set to a copy of the value.
 * If the key is not found, *out_val will remain.
 * @param key Key of the element to get, non-null.
 * @param out_val Pointer to returned value, non-null.
 * *out_val won't be freed in this function.
 * Caller is responsible to free *out_val after using.
 * @param out_val_len Output; byte size of *out_val.
 * @param out_age Output; age of this element in seconds.
 * @return Number of valid elements we get.
 */
int cache_get(const char* key,
              char** out_val,
              int* out_len,
              int* out_age)
{
    /* TODO */
    (void)key;
    (void)out_val;
    (void)out_len;
    (void)out_age;
    return 0;
}

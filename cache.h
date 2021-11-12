/**************************************************************
*
*                         cache.h
*
*     Final Project: High Performance HTTP Proxy
*     Author:  Keren Zhou (kzhou), Ruiyuan Gu (rgu03)
*     Date: 2021-11-11
*
*     Summary:
*     Interface for fixed size LRU cache.
*
**************************************************************/

#ifndef CACHE_H
#define CACHE_H

/**
 * @brief Initialize an empty cache of the given capacity.
 *
 * @param capacity Max size of the cache, > 0.
 * @return 0 on success; -1 otherwise.
 */
int cache_init(int capacity);

/**
 * Free the cache.
 */
void cache_clear(void);

/**
 * Put the given element (key, val, ttl) into cache.
 *
 * @param key Key of the element to be put, non-null.
 * @param val Value of the element to be put, non-null.
 * @param val_len Byte size of val.
 * @param max_age Time-to-live of the element in seconds, >= 0.
 * @return Number of elements put into cache.
 */
int cache_put(const char* key,
              const char* val,
              const int val_len,
              const int max_age);

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
              int* out_val_len,
              int* out_age);

#endif /* CACHE_H */

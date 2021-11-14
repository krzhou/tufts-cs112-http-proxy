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
#include "logger.h"
#include <stdlib.h>
#include <time.h>

struct cache_elem {
    char* key;
    char* val;
    int val_len; /* Byte size of val. */
    time_t creation_time; /* Creation time in seconds. */
    time_t max_age; /* Time-to-live in seconds. */
    struct cache_elem* next;
    struct cache_elem* prev;
};
typedef struct cache_elem cache_elem;

/**
 * @brief Create a new cache element.
 *
 * @param key Element key.
 * @param val Element value.
 * @param val_len Byte size of element value.
 * @param max_age Time-to-live in seconds.
 * @return cache_elem* Newly created cache element on success; NULL otherwise.
 */
cache_elem* cache_elem_new(const char* key,
                           const char* val,
                           const int val_len,
                           const time_t max_age)
{
    time_t now = time(NULL);

    cache_elem* elem = (cache_elem*)malloc(sizeof(cache_elem));
    if (elem == NULL) {
        PLOG_ERROR("malloc");
        return NULL;
    }
    if (key != NULL) {
        elem->key = strdup(key);
    }
    else {
        elem->key = NULL;
    }
    if (val != NULL) {
        elem->val = NULL;
        elem->val = malloc(val_len);
        if (elem->val == NULL) {
            PLOG_ERROR("fail to malloc elem->val");
            return NULL;
        }
        memcpy(elem->val, val, val_len);
        elem->val_len = val_len;
    }
    else {
        elem->val = NULL;
        elem->val_len = 0;
    }
    elem->creation_time = now;
    elem->max_age = max_age;
    elem->prev = NULL;
    elem->next = NULL;
    return elem;
}

/**
 * @brief Free the give cache element.
 *
 * @param elem Element to free.
 */
void cache_elem_free(cache_elem** elem)
{
    if (elem == NULL || *elem == NULL) {
        return;
    }
    free((*elem)->key);
    free((*elem)->val);
    free(*elem);
    *elem = NULL;
}

/**
 * @brief Get age of the given cache element in seconds.
 * 
 * @param elem Cache element.
 * @return time_t Age in seconds.
 */
time_t cache_elem_age(cache_elem* elem)
{
    return time(NULL) - elem->creation_time;
}

/**
 * @brief Check whether the age of the given cache element exceeds its max age.
 *
 * @param elem Cache element.
 * @return int 1 if age >= max_age; 0 otherwise.
 */
int cache_elem_is_stale(cache_elem* elem)
{
    return cache_elem_age(elem) >= elem->max_age;
}

struct cache {
    int size;
    int capacity;
    /* Doubly linked list of cache elements. */
    struct cache_elem* front;
    struct cache_elem* back;
};
typedef struct cache cache;

cache* the_cache = NULL; /* Global singleton cache. */

/**
 * @brief Initialize an empty cache of the given capacity.
 *
 * @param capacity Max size of the cache, > 0.
 * @return 0 on success; -1 otherwise.
 */
int cache_init(int capacity)
{
    cache_elem* dummy_front;
    cache_elem* dummy_back;

    if (capacity <= 0 || the_cache != NULL) {
        /* Invalid capacity or the cache has already been initialized. */
        return -1;
    }

    the_cache = (cache*)malloc(sizeof(cache));
    if (the_cache == NULL) {
        PLOG_ERROR("malloc");
        return -1;
    }
    the_cache->capacity = capacity;
    the_cache->size = 0;

    /* Create dummy nodes at front and back. Then, the doubly linked list won't
     * be empty. It facilities insertions and removals. */
    dummy_front = cache_elem_new(NULL, NULL, 0, 0);
    if (dummy_front == NULL) {
        PLOG_ERROR("malloc");
        free(the_cache);
        the_cache = NULL;
        return -1;
    }
    dummy_back = cache_elem_new(NULL, NULL, 0, 0);
    if (dummy_back == NULL) {
        PLOG_ERROR("malloc");
        free(dummy_front);
        free(the_cache);
        the_cache = NULL;
        return -1;
    }
    dummy_front->prev = NULL;
    dummy_front->next = dummy_back;
    dummy_back->next = NULL;
    dummy_back->prev = dummy_front;
    the_cache->front = dummy_front;
    the_cache->back = dummy_back;
    return 0;
}

/**
 * Free the cache.
 */
void cache_clear(void)
{
    cache_elem* curr;
    cache_elem* next;

    if (the_cache == NULL) {
        return;
    }

    curr = the_cache->front;
    while (curr != NULL) {
        next = curr->next;
        cache_elem_free(&curr);
        curr = next;
    }
    free(the_cache);
    the_cache = NULL;
}

/**
 * Get the element by the given key from cache, no matter it is valid or not.
 *
 * @param key Key of the element to get, non-null.
 * @return A pointer to the element of the given key if found; otherwise, NULL.
 */
cache_elem* cache_force_get_elem(const char* key)
{
    cache_elem* elem;

    /* Invalid args. */
    if (the_cache == NULL || key == NULL) {
        return NULL;
    }

    elem = the_cache->front->next;
    while (elem != the_cache->back) {
        if (strcmp(elem->key, key) == 0) {
            return elem;
        }
        elem = elem->next;
    }
    return NULL;
}

/**
 * Update the element of the given key.
 *
 * @param key Key of the element to be updated, non-null.
 * @param val Value of the element to be updated, non-null.
 * @param val_len Byte size of val.
 * @param max_age Time-to-live of the element in seconds, >= 0.
 * @return Number of elements updated in cache.
 */
int cache_update(const char* key,
                 const char* val,
                 const int val_len,
                 const int max_age)
{
    cache_elem* elem;

    /* Validate args. */
    if (the_cache == NULL || key == NULL || val == NULL || val_len < 0) {
        return 0;
    }

    elem = cache_force_get_elem(key);
    if (elem == NULL) {
        return 0;
    }
    /* Update element contents. */
    free(elem->val);
    elem->val = NULL;
    elem->val = malloc(val_len);
    if (elem->val == NULL) {
        PLOG_ERROR("malloc");
        return 0;
    }
    memcpy(elem->val, val, val_len);
    elem->val_len = val_len;
    elem->creation_time = time(NULL);
    elem->max_age = max_age;
    /* Move the updated element to the front. */
    /* Detach the update element. */
    elem->prev->next = elem->next;
    elem->next->prev = elem->prev;
    /* Insert the updated element at the front. */
    elem->next = the_cache->front->next;
    the_cache->front->next->prev = elem;
    elem->prev = the_cache->front;
    the_cache->front->next = elem;
    return 1;
}

/**
 * Remove the given element from cache, regardless of whether element is valid
 * and in cache.
 *
 * @param elem Element to remove, non-null.
 * @return Number of removed elements.
 */
int cache_force_remove_elem(cache_elem** elem)
{
    /* Validate args. */
    if (the_cache == NULL || elem == NULL || *elem == NULL) {
        return 0;
    }

    (*elem)->prev->next = (*elem)->next;
    (*elem)->next->prev = (*elem)->prev;
    cache_elem_free(elem);
    (the_cache->size)--;
    return 1;
}

/**
 * Remove and free all the stale elements in cache.
 * @return Number of removed elements.
 */
int cache_remove_all_stale(void)
{
    int count = 0;
    cache_elem* curr;
    cache_elem* next;

    if (the_cache == NULL) {
        return 0;
    }

    curr = the_cache->front->next;
    while (curr != the_cache->back) {
        next = curr->next;
        if (cache_elem_is_stale(curr)) {
            cache_force_remove_elem(&curr);
            count++;
        }
        curr = next;
    }
    return count;
}

/**
 * Remove and free the last element in cache.
 *
 * @return Number of elements removed.
 */
int cache_pop_back(void)
{
    cache_elem* last;

    /* Invalid args. */
    if (the_cache == NULL || the_cache->size <= 0) {
        return 0;
    }

    last = the_cache->back->prev;
    last->prev->next = last->next;
    last->next->prev = last->prev;
    cache_elem_free(&last);
    (the_cache->size)--;
    return 1;
}

/**
 * Add element at the front of cache, regardless of capacity.
 *
 * @param elem Element to remove, non-null.
 * @return Number of elements added.
 */
int cache_force_push_front(cache_elem* elem)
{
    /* Validate args. */
    if (the_cache == NULL || elem == NULL) {
        return 0;
    }

    elem->next = the_cache->front->next;
    the_cache->front->next->prev = elem;
    elem->prev = the_cache->front;
    the_cache->front->next = elem;
    (the_cache->size)++;
    return 1;
}

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
              const int max_age)
{
    cache_elem* elem = NULL;

    /* Invalid args. */
    if (the_cache == NULL || key == NULL || val == NULL || val_len < 0) {
        return 0;
    }

    /* If KEY is found in CACHE, update the element. */
    if (cache_update(key, val, val_len, max_age) > 0) {
        return 1;
    }

    elem = cache_elem_new(key, val, val_len, max_age);
    /* If CACHE is full, remove stale elements first. */
    if (the_cache->size == the_cache->capacity &&
        cache_remove_all_stale() == 0) {
        /* If no stale element, remove the least recently used element. */
        cache_pop_back();
    }
    /* Add the new element to the front. */
    cache_force_push_front(elem);
    return 1;
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
              int* out_val_len,
              int* out_age)
{
    cache_elem* elem = NULL;

    /* Validate args. */
    if (the_cache == NULL ||
        key == NULL ||
        out_val == NULL ||
        out_val_len == NULL ||
        out_age == NULL) {

        return 0;
    }

    elem = cache_force_get_elem(key);
    if (elem == NULL) {
        return 0;
    }
    /* Remove the stale element. */
    if (cache_elem_is_stale(elem)) {
        cache_force_remove_elem(&elem);
        return 0;
    }
    *out_val = NULL;
    *out_val = malloc(elem->val_len);
    memcpy(*out_val, elem->val, elem->val_len);
    *out_val_len = elem->val_len;
    *out_age = cache_elem_age(elem);
    return 1;
}

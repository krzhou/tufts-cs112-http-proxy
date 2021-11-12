/**************************************************************
*
*                       test_cache.c
*
*     Final Project: High Performance HTTP Proxy
*     Author:  Keren Zhou (kzhou), Ruiyuan Gu (rgu03)
*     Date: 2021-11-11
*
*     Summary:
*     Test driver for fixed size LRU cache.
*
**************************************************************/

#include "cache.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

cache_elem* cache_elem_new(const char* key,
                           const char* val,
                           const int val_len,
                           const time_t max_age);
void cache_elem_free(cache_elem** elem);
time_t cache_elem_age(cache_elem* elem);
int cache_elem_is_stale(cache_elem* elem);

void test_cache_elem_new_normal(void)
{
    char* key;
    char* val;
    int val_len;
    time_t max_age;
    time_t creation_time;
    cache_elem* elem;

    fprintf(stderr, "--------------------\n");
    fprintf(stderr, "TEST cache_elem_new() normal\n");
    key = "Networks";
    val = "Tufts University Fall 2021";
    val_len = strlen(val) + 1;
    max_age = 10;
    creation_time = time(NULL);
    elem = cache_elem_new(key, val, val_len, max_age);
    assert(elem != NULL);
    assert(strcmp(elem->key, key) == 0);
    assert(strncmp(elem->val, val, val_len) == 0);
    assert(elem->val_len == val_len);
    assert(elem->creation_time == creation_time);
    assert(elem->max_age == max_age);
    assert(elem->prev == NULL);
    assert(elem->next == NULL);
    fprintf(stderr, "PASS\n");
    cache_elem_free(&elem);
    fprintf(stderr, "--------------------\n");
}

void test_cache_elem_null_key_or_val(void)
{
    char* key;
    char* val;
    int val_len;
    time_t max_age;
    time_t creation_time;
    cache_elem* elem;

    fprintf(stderr, "--------------------\n");
    fprintf(stderr, "TEST cache_elem_new() NULL key or val\n");
    key = NULL;
    val = NULL;
    val_len = 0;
    max_age = 0;
    creation_time = time(NULL);
    elem = cache_elem_new(key, val, val_len, max_age);
    assert(elem != NULL);
    assert(elem->key == NULL);
    assert(elem->val == NULL);
    assert(elem->val_len == val_len);
    assert(elem->creation_time == creation_time);
    assert(elem->max_age == max_age);
    assert(elem->prev == NULL);
    assert(elem->next == NULL);
    fprintf(stderr, "PASS\n");
    cache_elem_free(&elem);
    fprintf(stderr, "--------------------\n");
}

void test_cache_elem_empty_str_key_or_val(void)
{
    char* key;
    char* val;
    int val_len;
    time_t max_age;
    time_t creation_time;
    cache_elem* elem;

    fprintf(stderr, "--------------------\n");
    fprintf(stderr, "TEST cache_elem_new() empty string key or val\n");
    key = "";
    val = "";
    val_len = 1;
    max_age = 0;
    creation_time = time(NULL);
    elem = cache_elem_new(key, val, val_len, max_age);
    assert(elem != NULL);
    assert(strcmp(elem->key, key) == 0);
    assert(strncmp(elem->val, val, val_len) == 0);
    assert(elem->val_len == val_len);
    assert(elem->creation_time == creation_time);
    assert(elem->max_age == max_age);
    assert(elem->prev == NULL);
    assert(elem->next == NULL);
    fprintf(stderr, "PASS\n");
    cache_elem_free(&elem);
    fprintf(stderr, "--------------------\n");
}

void test_cache_elem_new(void)
{
    test_cache_elem_new_normal();
    test_cache_elem_null_key_or_val();
    test_cache_elem_empty_str_key_or_val();
}

void test_cache_elem_free_normal(void)
{
    char* key;
    char* val;
    int val_len;
    time_t max_age;
    cache_elem* elem;

    fprintf(stderr, "--------------------\n");
    fprintf(stderr, "TEST cache_elem_free() normal\n");
    key = "Networks";
    val = "Tufts University Fall 2021";
    val_len = strlen(val) + 1;
    max_age = 10;
    elem = cache_elem_new(key, val, val_len, max_age);
    cache_elem_free(&elem);
    assert(elem == NULL);
    fprintf(stderr, "PASS\n");
    fprintf(stderr, "--------------------\n");
}

void test_cache_elem_free_null(void)
{
    fprintf(stderr, "--------------------\n");
    fprintf(stderr, "TEST cache_elem_free(NULL)\n");
    cache_elem_free(NULL);
    /* Should have no error. */
    fprintf(stderr, "PASS\n");
    fprintf(stderr, "--------------------\n");
}

void test_cache_elem_free_null_elem(void)
{
    cache_elem* elem;

    fprintf(stderr, "--------------------\n");
    fprintf(stderr, "TEST cache_elem_free() NULL elem\n");
    elem = NULL;
    cache_elem_free(&elem);
    assert(elem == NULL);
    fprintf(stderr, "PASS\n");
    fprintf(stderr, "--------------------\n");
}

void test_cache_elem_free_null_key_or_val(void)
{
    cache_elem* elem;

    fprintf(stderr, "--------------------\n");
    fprintf(stderr, "TEST cache_elem_free() NULL key or val\n");
    elem = cache_elem_new(NULL, NULL, 0, 0);
    assert(elem != NULL);
    cache_elem_free(&elem);
    assert(elem == NULL);
    fprintf(stderr, "PASS\n");
    fprintf(stderr, "--------------------\n");
}

void test_cache_elem_free_empty_str_key_or_val(void)
{
    cache_elem* elem;

    fprintf(stderr, "--------------------\n");
    fprintf(stderr, "TEST cache_elem_free() empty string key or val\n");
    elem = cache_elem_new("", "", 1, 0);
    assert(elem != NULL);
    cache_elem_free(&elem);
    assert(elem == NULL);
    fprintf(stderr, "PASS\n");
    fprintf(stderr, "--------------------\n");
}

void test_cache_elem_free(void)
{
    test_cache_elem_free_normal();
    test_cache_elem_free_null();
    test_cache_elem_free_null_elem();
    test_cache_elem_free_null_key_or_val();
    test_cache_elem_free_empty_str_key_or_val();
}

void test_cache_elem_age(void)
{
    char* key;
    char* val;
    int val_len;
    time_t max_age;
    time_t creation_time;
    cache_elem* elem;
    time_t expected_age;
    time_t actual_age;

    fprintf(stderr, "--------------------\n");
    fprintf(stderr, "TEST cache_elem_age() normal\n");
    key = "Networks";
    val = "Tufts University Fall 2021";
    val_len = strlen(val) + 1;
    max_age = 10;
    creation_time = 0;
    elem = cache_elem_new(key, val, val_len, max_age);
    assert(elem != NULL);
    elem->creation_time = creation_time;
    expected_age = time(NULL) - creation_time;
    actual_age = cache_elem_age(elem);
    fprintf(stderr,
            "expected age: %lu\n"
            "actual age: %lu\n",
            expected_age,
            actual_age);
    assert(expected_age == actual_age);
    fprintf(stderr, "PASS\n");
    cache_elem_free(&elem);
    fprintf(stderr, "--------------------\n");
}

void test_cache_elem_is_stale_true(void)
{
    char* key; char* val;
    int val_len;
    time_t max_age;
    cache_elem* elem;

    fprintf(stderr, "--------------------\n");
    fprintf(stderr, "TEST cache_elem_is_stale() true\n");
    key = "Networks";
    val = "Tufts University Fall 2021";
    val_len = strlen(val) + 1;
    max_age = 10;
    elem = cache_elem_new(key, val, val_len, max_age);
    assert(elem != NULL);
    assert(!cache_elem_is_stale(elem));
    fprintf(stderr, "PASS\n");
    cache_elem_free(&elem);
    fprintf(stderr, "--------------------\n");
}

void test_cache_elem_is_stale_false(void)
{
    char* key; char* val;
    int val_len;
    time_t max_age;
    cache_elem* elem;

    fprintf(stderr, "--------------------\n");
    fprintf(stderr, "TEST cache_elem_age() false\n");
    key = "Networks";
    val = "Tufts University Fall 2021";
    val_len = strlen(val) + 1;
    max_age = 10;
    elem = cache_elem_new(key, val, val_len, max_age);
    assert(elem != NULL);
    elem->creation_time += max_age;
    assert(!cache_elem_is_stale(elem));
    elem->creation_time += 1;
    assert(!cache_elem_is_stale(elem));
    fprintf(stderr, "PASS\n");
    cache_elem_free(&elem);
    fprintf(stderr, "--------------------\n");
}

void test_cache_elem_is_stale(void)
{
    test_cache_elem_is_stale_true();
    test_cache_elem_is_stale_false();
}

struct cache {
    int size;
    int capacity;
    /* Doubly linked list of cache elements. */
    struct cache_elem* front;
    struct cache_elem* back;
};
typedef struct cache cache;

cache* the_cache; /* Global singleton cache declearation. */

/* Assert that the cache is empty and in a valid state. */
void assert_cache_empty(void)
{
    cache_elem* front;
    cache_elem* back;

    assert(the_cache != NULL);
    assert(the_cache->size == 0);

    front = the_cache->front;
    assert(front != NULL);
    assert(front->key == NULL);
    assert(front->val == NULL);

    back = the_cache->back;
    assert(back != NULL);
    assert(back->key == NULL);
    assert(back->val == NULL);

    assert(front->prev == NULL);
    assert(front->next == back);
    assert(back->prev == front);
    assert(back->next == NULL);
}

void test_cache_init_normal(void)
{
    int capacity;

    fprintf(stderr, "--------------------\n");
    fprintf(stderr, "TEST cache_init() normal\n");
    capacity = 10;
    assert(cache_init(capacity) == 0);
    assert_cache_empty();
    assert(the_cache->capacity == capacity);
    fprintf(stderr, "PASS\n");
    cache_clear();
    fprintf(stderr, "--------------------\n");
}

void test_cache_init_neg_cap(void)
{
    fprintf(stderr, "--------------------\n");
    fprintf(stderr, "TEST cache_new() capacity < 0\n");
    assert(cache_init(-1) < 0);
    assert(the_cache == NULL);
    fprintf(stderr, "PASS\n");
    fprintf(stderr, "--------------------\n");
}

void test_cache_init_0_cap(void)
{
    fprintf(stderr, "--------------------\n");
    fprintf(stderr, "TEST cache_new() capacity == 0\n");
    assert(cache_init(0) < 0);
    assert(the_cache == NULL);
    fprintf(stderr, "PASS\n");
    fprintf(stderr, "--------------------\n");
}

void test_cache_init(void)
{
    test_cache_init_normal();
    test_cache_init_neg_cap();
    test_cache_init_0_cap();
}

/* Assert properties of the given cache element. */
void assert_cache_elem(const cache_elem* elem,
                       const char* key,
                       const char* val,
                       const int val_len,
                       const time_t creation_time,
                       const time_t max_age)
{
    assert(elem != NULL);
    assert((key == NULL && elem->key == NULL) ||
           strcmp(elem->key, key) == 0);
    assert((val == NULL && elem->val == NULL) ||
           strncmp(elem->val, val, val_len) == 0);
    assert(elem->val_len == val_len);
    assert(elem->creation_time == creation_time);
    assert(elem->max_age == max_age);
}

void test_cache_put_add(void)
{
    time_t creation_time;
    char* key1;
    char* val1;
    int val_len1;
    time_t max_age1;
    time_t creation_time1;
    char* key2;
    char* val2;
    int val_len2;
    time_t max_age2;
    time_t creation_time2;

    fprintf(stderr, "--------------------\n");
    fprintf(stderr, "TEST cache_put() add 2 elements\n");
    creation_time = time(NULL);
    assert(cache_init(10) == 0);

    /* Add one element. */
    key1 = "key1";
    val1 = "value1";
    val_len1 = strlen(val1) + 1;
    max_age1 = 100;
    creation_time1 = time(NULL);
    assert(cache_put(key1, val1, val_len1, max_age1) == 1);
    assert(the_cache->size == 1);
    /* Check dummy front and back. */
    assert_cache_elem(the_cache->front, NULL, NULL, 0, creation_time, 0);
    assert_cache_elem(the_cache->back, NULL, NULL, 0, creation_time, 0);
    /* Check elements. */
    assert_cache_elem(the_cache->front->next,
                      key1,
                      val1,
                      val_len1,
                      creation_time1,
                      max_age1);
    assert(the_cache->front->next == the_cache->back->prev);

    /* Add another element. */
    key2 = "key2";
    val2 = "value2";
    val_len2 = strlen(val2) + 1;
    max_age2 = 200;
    creation_time2 = time(NULL);
    assert(cache_put(key2, val2, val_len2, max_age2) == 1);
    assert(the_cache->size == 2);
    /* Check dummy front and back. */
    assert_cache_elem(the_cache->front, NULL, NULL, 0, creation_time, 0);
    assert_cache_elem(the_cache->back, NULL, NULL, 0, creation_time, 0);
    /* Check elements. */
    assert_cache_elem(the_cache->front->next,
                      key2,
                      val2,
                      val_len2,
                      creation_time2,
                      max_age2);
    assert_cache_elem(the_cache->back->prev,
                      key1,
                      val1,
                      val_len1,
                      creation_time1,
                      max_age1);
    assert(the_cache->front->next->next == the_cache->back->prev);

    cache_clear();
    fprintf(stderr, "PASS\n");
    fprintf(stderr, "--------------------\n");
}

void test_cache_put(void)
{
    /* TODO */
    // test_cache_put_invalid_args();
    test_cache_put_add();
    // test_cache_put_update_valid();
    // test_cache_put_update_stale();
    // test_cache_put_full_clean_stale();
    // test_cache_put_full_pop_back();
}

void test_cache_get(void)
{
    /* TODO */
}

void test_cache_clear(void)
{
    /* TODO */
}

int main(void)
{
    fprintf(stderr, "====================\n");
    test_cache_elem_new();
    test_cache_elem_free();
    test_cache_elem_age();
    test_cache_elem_is_stale();

    test_cache_init();
    test_cache_put();
    test_cache_get();
    test_cache_clear();

    fprintf(stderr, "ALL PASS\n");
    fprintf(stderr, "====================\n\n");
    return EXIT_SUCCESS;
}

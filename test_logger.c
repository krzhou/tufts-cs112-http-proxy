/**
 * test_logger.c
 *
 * Auther: Keren Zhou (kzhou)
 * Date 2021-10-20
 *
 * Summary:
 * Test driver for log utilities.
 */

#include "logger.h"
#include <stdio.h>
#include <stdlib.h>

void test_print_log(void)
{
    fprintf(stderr, "---- TEST LOG_INFO ----\n");

    /* Normal. */
    print_log(__FILE__, __LINE__, "Hello, world!");

    /* Empty string. */
    print_log(__FILE__, __LINE__, "");

    /* Long string. */
    print_log(__FILE__,
              __LINE__,
              "a very long log that exceeds 128 bytes"
              "a very long log that exceeds 128 bytes"
              "a very long log that exceeds 128 bytes"
              "a very long log that exceeds 128 bytes"
              "a very long log that exceeds 128 bytes");

    fprintf(stderr, "PASS\n");
    fprintf(stderr, "--------------------------\n\n");
}

void test_LOG_INFO(void)
{
    fprintf(stderr, "---- TEST LOG_INFO ----\n");

    /* Normal. */
    LOG_INFO( "Hello, world!");

    /* Empty string. */
    LOG_INFO("");

    /* Long string. */
    LOG_INFO("a very long log that exceeds 128 bytes"
             "a very long log that exceeds 128 bytes"
             "a very long log that exceeds 128 bytes"
             "a very long log that exceeds 128 bytes"
             "a very long log that exceeds 128 bytes");

    fprintf(stderr, "PASS\n");
    fprintf(stderr, "--------------------------\n\n");
}

void test_LOG_ERROR(void)
{
    fprintf(stderr, "---- TEST LOG_ERROR ----\n");

    /* Normal. */
    LOG_ERROR( "Hello, world!");

    /* Empty string. */
    LOG_ERROR("");

    /* Long string. */
    LOG_ERROR("a very long log that exceeds 128 bytes"
              "a very long log that exceeds 128 bytes"
              "a very long log that exceeds 128 bytes"
              "a very long log that exceeds 128 bytes"
              "a very long log that exceeds 128 bytes");

    fprintf(stderr, "PASS\n");
    fprintf(stderr, "--------------------------\n\n");
}

void test_PLOG_ERROR(void)
{
    fprintf(stderr, "---- TEST PLOG_ERROR ----\n");

    /* Normal. */
    PLOG_ERROR("Hello, world!");

    /* Empty string. */
    PLOG_ERROR("");

    /* Long string. */
    PLOG_ERROR("a very long log that exceeds 128 bytes"
              "a very long log that exceeds 128 bytes"
              "a very long log that exceeds 128 bytes"
              "a very long log that exceeds 128 bytes"
              "a very long log that exceeds 128 bytes");

    fprintf(stderr, "PASS\n");
    fprintf(stderr, "--------------------------\n\n");
}

void test_LOG_FATAL(void)
{
    fprintf(stderr, "---- TEST LOG_FATAL ----\n");

    /* Normal. */
    LOG_FATAL( "Hello, world!");

    /* Empty string. */
    LOG_FATAL("");

    /* Long string. */
    LOG_FATAL("a very long log that exceeds 128 bytes"
              "a very long log that exceeds 128 bytes"
              "a very long log that exceeds 128 bytes"
              "a very long log that exceeds 128 bytes"
              "a very long log that exceeds 128 bytes");

    fprintf(stderr, "PASS\n");
    fprintf(stderr, "--------------------------\n\n");
}

void test_PLOG_FATAL(void)
{
    fprintf(stderr, "---- TEST PLOG_FATAL ----\n");

    /* Normal. */
    PLOG_FATAL( "Hello, world!");

    /* Empty string. */
    PLOG_FATAL("");

    /* Long string. */
    PLOG_FATAL("a very long log that exceeds 128 bytes"
              "a very long log that exceeds 128 bytes"
              "a very long log that exceeds 128 bytes"
              "a very long log that exceeds 128 bytes"
              "a very long log that exceeds 128 bytes");

    fprintf(stderr, "PASS\n");
    fprintf(stderr, "--------------------------\n\n");
}

int main(void)
{
    fprintf(stderr, "==== TEST logger ====\n");

    test_print_log();
    test_LOG_INFO();
    test_LOG_ERROR();
    test_PLOG_ERROR();
    // test_LOG_FATAL();
    // test_PLOG_FATAL();

    fprintf(stderr, "ALL PASS\n");
    fprintf(stderr, "==========================\n\n");

    return EXIT_SUCCESS;
}
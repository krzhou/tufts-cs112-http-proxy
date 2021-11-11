/**
 * logger.h
 *
 * Auther: Keren Zhou (kzhou)
 * Date 2021-10-19
 *
 * Summary:
 * Interface for log utilities.
 */

#include "logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

/**
 * @brief Print log message with source file path and line number to stderr.
 *
 * @param file Source file path.
 * @param line Line number in the source file.
 * @param fmt Log format.
 */
void print_log(const char *file, int line, const char *fmt, ...)
{
    va_list args = {0};
    char* msg = NULL; /* Custom log message to print. */
    size_t size = 0; /* Byte size of msg including '\0'. */

    /* Get actual message size including '\0'. */
    va_start(args, fmt);
    size = vsnprintf(msg, size, fmt, args) + 1;
    va_end(args);

    /* Expand message. */
    msg = malloc(size);
    if (msg == NULL) {
        fprintf(stderr,
                "%s:%d: malloc failed with size %zu\n",
                __FILE__,
                __LINE__,
                size);
        exit(EXIT_FAILURE);
    }
    va_start(args, fmt);
    vsnprintf(msg, size, fmt, args);
    va_end(args);

    /* Print log message with source file path and line number. */
    fprintf(stderr, "%s:%d: %s\n", file, line, msg);

    free(msg);
}
/**
 * logger.h
 *
 * Auther: Keren Zhou (kzhou)
 * Date 2021-10-19
 *
 * Summary:
 * Interface for log utilities.
 */

#ifndef LOGGER_H
#define LOGGER_H

#include <errno.h>
#include <string.h>

/**
 * @brief Print log message with source file path and line number to stderr.
 * 
 * @param file Source file path.
 * @param line Line number in the source file.
 * @param fmt Log format.
 */
void print_log(const char* file, int line, const char* fmt, ...);

#define LOG_RED "\x1B[31m"
#define LOG_NORMAL "\x1B[0m"

/**
 * @brief Print message to stderr.
 *
 * @param fmt Message format.
 */
#define LOG_INFO(fmt, ...) print_log(__FILE__, __LINE__, fmt, ##__VA_ARGS__)

/**
 * @brief Print error message to stderr.
 *
 * @param fmt Message format.
 */
#define LOG_ERROR(fmt, ...)                                                    \
        print_log(__FILE__,                                                    \
                  __LINE__,                                                    \
                  LOG_RED "error: " LOG_NORMAL fmt,                            \
                  ##__VA_ARGS__)

/**
 * @brief Print fatal message to stderr and exit on failure.
 *
 * @param fmt Message format.
 */
#define LOG_FATAL(fmt, ...)                                                    \
        do {                                                                   \
            print_log(__FILE__,                                                \
                      __LINE__,                                                \
                      LOG_RED "fatal: " LOG_NORMAL fmt,                        \
                      ##__VA_ARGS__);                                          \
            exit(1);                                                           \
        } while(0)

/**
 * @brief Print error message with errno to stderr.
 *
 * @param fmt Message format.
 */
#define PLOG_ERROR(fmt, ...)                                                   \
        LOG_ERROR(fmt ": %s", ##__VA_ARGS__, strerror(errno))

/**
 * @brief Print fatal message with errno to stderr and exit on failure.
 *
 * @param fmt Message format.
 */
#define PLOG_FATAL(fmt, ...)                                                   \
        LOG_FATAL(fmt ": %s", ##__VA_ARGS__, strerror(errno))

#endif /* LOGGER_H */
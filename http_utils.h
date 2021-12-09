/**************************************************************
*
*                       http_parser.h
*
*     Final Project: High Performance HTTP Proxy
*     Author:  Keren Zhou (kzhou), Ruiyuan Gu (rgu03)
*     Date: 2021-11-12
*
*     Summary:
*     Interface for http parser.
*
**************************************************************/

#ifndef HTTP_PARSER_H
#define HTTP_PARSER_H

/**
 * @brief Parse HTTP request/response and extract its head and body.
 *
 * @param buf Byte buffer should contain a whole HTTP request/response head and
 * may contain a whole/partial/empty entity body.
 * @param n Byte size of the buffer.
 * @param out_head Output pointer to a null-terminated string copy of head
 * without the empty line.
 * If the empty line between head and body is not found, *out_head will remain
 * its original value.
 * @param out_head_len Output; byte size of head.
 * @param out_body Output pointer to a null-terminated string copy of entity
 * body.
 * It may not be the whole entity body, or even be an empty string.
 * If empty line between head and body is not found, *out_body will remain its
 * original value.
 * @param out_body_len Output; byte size of body.
 */
void parse_body_head(const char* buf,
                     const int n,
                     char** out_head,
                     int* out_head_len,
                     char** out_body,
                     int* out_body_len);

/**
 * @brief Parse the given HTTP request and extract method, url, version and host
 * fields.
 *
 * @param request HTTP request to parse.
 * @param out_method Output pointer to a null-terminated string as a copy of
 * method field in the request.
 * @param out_url Output pointer to a null-terminated string as a copy of url
 * field in the request.
 * @param out_version Output pointer to a null-terminated string as a copy of
 * version field in the request.
 * @param host Output pointer to a null-terminated string as a copy of host
 * field in the requst.
 */
void parse_request_head(const char* request,
                        char** out_method,
                        char** out_url,
                        char** out_version,
                        char** out_host);

/**
 * @brief Parse the given host field value of an HTTP request, and extract
 * hostname and port number.
 *
 * @param host Host field value in an HTTP request. It may not contain port
 * number.
 * @param out_hostname Output pointer to a string copy of hostname without port
 * number.
 * @param out_port Output pointer to an integer copy of port number.
 * If port number is not specified in host field, out_port remains its original
 * value.
 */
void parse_host_field(const char* host, char** out_hostname, int* out_port);

/**
 * @brief Parse the given HTTP status line and extract version, status code and 
 * phrase fields.
 *
 * @param line String that starts with HTTP status line to parse. It may contain
 * other contents after the status line.
 * @param out_version Output pointer to a string copy of version field.
 * @param out_status_code Output pointer to an integer copy of status code
 * field.
 * @param out_phrase Pointer to a string copy of phrase field.
 * @return Length of status line including "\r\n"; -1 if the given request line
 * is invalid.
 */
int parse_status_line(const char* line,
                      char** out_version,
                      int* out_status_code,
                      char** out_phrase);

/**
 * @brief Parse the given HTTP response, extract version, status code, phrase, 
 * content length and cache_control fields.
 *
 * @param response String that starts with HTTP response to parse. It contains
 * the whole response header. But it may not contain the whole entity body.
 * @param out_version Output pointer to a string copy of version field.
 * @param out_status_code Output pointer to an integer copy of status code
 * field.
 * @param out_phrase Output pointer to a string copy of phrase field.
 * @param out_content_length Output pointer to a string copy of content length
 * field.
 * @param out_cache_control Output pointer to a string copy of cache control
 * field.
 */
void parse_response_head(const char* response,
                         char** out_version,
                         int* out_status_code,
                         char** out_phrase,
                         int* out_content_length,
                         char** out_cache_control);

/**
 * @brief Parse the given cache control field and extract the integer after 
 * "max-age=".
 *
 * @param cache_control String of cache control field.
 * @param out_max_age Ouput; Integer after "max-age=".
 * If "max-age=" is not found, *out_max_age will remain its original value.
 */
void parse_cache_control(const char* cache_control, int* out_max_age);

/**
 * @brief Extract the first complete HTTP request from buf.
 * 
 * @param buf Buffer may contain a HTTP request.
 * @param n Byte size of the buffer.
 * @param out_request Output: The first HTTP request head in buffer if the 
 * request is completed; it is not changed otherwise.
 * @param out_len Output; Byte size of request head if it is completed; it is
 * not changed otherwise.
 * @return int Number of extracted request, i.e. 1 on success; 0 otherwise.
 */
int extract_first_request(char** buf,
                          int* n,
                          char** out_request,
                          int* out_len);

/**
 * @brief Extract the first complete HTTP response from buf.
 * 
 * @param buf Buffer may contain a HTTP response.
 * @param n Byte size of the buffer.
 * @param out_response Output: String of the first HTTP response in buffer if the
 * response is completed; it is not changed otherwise.
 * @param out_len Output; Byte size of response if it is completed; it is not
 * changed otherwise.
 * @param out_max_age Output: Max age (time-to-live) for the response in cache.
 * @return int Number of extracted response, i.e. 1 on success; 0 otherwise.
 */
int extract_first_response(char** buf,
                           int* n,
                           char** out_request,
                           int* out_len,
                           int* out_max_age,
                           int* is_chunked);

#endif /* HTTP_PARSER_H */

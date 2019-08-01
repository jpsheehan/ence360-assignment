#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

#include "http.h"

#define BUF_SIZE 1024

#define HEADER_USER_AGENT "ENCE360/1.0 (assignment)"

enum http_method
{
    GET,
    POST,
    HEAD
};
typedef enum http_method HttpMethod;

/**
 * Creates a buffer with size t_initial_size bytes.
 * Returns a pointer to the buffer or NULL upon failure.
 */
Buffer *buffer_create(size_t t_initial_size)
{
    Buffer *buffer = malloc(sizeof(Buffer));

    if (buffer)
    {
        buffer->data = malloc(t_initial_size);

        if (buffer->data == NULL)
        {
            free(buffer);
            buffer = NULL;
        }
        else
        {
            buffer->length = t_initial_size;
        }
    }

    return buffer;
}

/**
 * Doubles the capacity of the buffer.
 * Returns 0 on success, -1 on failure.
 */
int buffer_double_size(Buffer *t_buffer)
{
    size_t new_length = t_buffer->length * 2;
    char *new_data = realloc(t_buffer->data, new_length);

    if (new_data != t_buffer->data)
    {
        t_buffer->data = new_data;
        t_buffer->length = new_length;
        return 0;
    }
    else
    {
        return -1;
    }
}

Buffer *http_raw_query(char *host, char *path, HttpMethod method, const char *range, int port)
{
    assert(0 && "not implemented yet!");
}

/**
 * Perform an HTTP 1.0 query to a given host and page and port number.
 * host is a hostname and page is a path on the remote server. The query
 * will attempt to retrievev content in the given byte range.
 * User is responsible for freeing the memory.
 * 
 * @param host - The host name e.g. www.canterbury.ac.nz
 * @param page - e.g. /index.html
 * @param range - Byte range e.g. 0-500. NOTE: A server may not respect this
 * @param port - e.g. 80
 * @return Buffer - Pointer to a buffer holding response data from query
 *                  NULL is returned on failure.
 */
Buffer *http_query(char *host, char *page, const char *range, int port)
{
    return http_raw_query(host, page, GET, range, port);
}

/**
 * Separate the content from the header of an http request.
 * NOTE: returned string is an offset into the response, so
 * should not be freed by the user. Do not copy the data.
 * @param response - Buffer containing the HTTP response to separate 
 *                   content from
 * @return string response or NULL on failure (buffer is not HTTP response)
 */
char *http_get_content(Buffer *response)
{

    char *header_end = strstr(response->data, "\r\n\r\n");

    if (header_end)
    {
        return header_end + 4;
    }
    else
    {
        return response->data;
    }
}

/**
 * Splits an HTTP url into host, page. On success, calls http_query
 * to execute the query against the url. 
 * @param url - Webpage url e.g. learn.canterbury.ac.nz/profile
 * @param range - The desired byte range of data to retrieve from the page
 * @return Buffer pointer holding raw string data or NULL on failure
 */
Buffer *http_url(const char *url, const char *range)
{
    char host[BUF_SIZE];
    strncpy(host, url, BUF_SIZE);

    char *page = strstr(host, "/");

    if (page)
    {
        page[0] = '\0';

        ++page;
        return http_query(host, page, range, 80);
    }
    else
    {

        fprintf(stderr, "could not split url into host/page %s\n", url);
        return NULL;
    }
}

/**
 * Makes a HEAD request to a given URL and gets the content length
 * Then determines max_chunk_size and number of split downloads needed
 * @param url   The URL of the resource to download
 * @param threads   The number of threads to be used for the download
 * @return int  The number of downloads needed satisfying max_chunk_size
 *              to download the resource
 */
int get_num_tasks(char *url, int threads)
{
    assert(0 && "not implemented yet!");
}

int get_max_chunk_size()
{
    return max_chunk_size;
}

#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdbool.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

#include "http.h"

#define BUF_SIZE 1024

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

        // initialise the new memory to 0
        memset(t_buffer->data + t_buffer->length, 0, t_buffer->length);

        t_buffer->length = new_length;
        return 0;
    }
    else
    {
        return -1;
    }
}

/**
 * Allocates memory for a null-terminated string containing the request. The
 * string must be freed manually.
 * Returns NULL upon failure.
 */
Buffer *util_create_request(const char *t_path)
{
    // "GET" + " " + $PATH + " " + "HTTP/1.0" + "\r\n" + "\r\n" + "\0"
    size_t length = 3 + 1 + strlen(t_path) + 1 + 8 + 2 + 2 + 1;
    Buffer *buffer = buffer_create(length);

    if (buffer != NULL)
    {
        snprintf(buffer->data, length, "GET %s HTTP/1.0\r\n\r\n", t_path);
    }

    return buffer;
}

/**
 * Creates and returns a client socket. Based on https://gist.github.com/browny/5211329
 * Returns -1 upon failure.
 */
int util_create_socket(const char *t_host, int t_port)
{
    int sockfd = 0;

    // attempt to create the socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) >= 0)
    {
        struct sockaddr_in serv_addr;
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(t_port);

        if (inet_pton(AF_INET, t_host, &serv_addr.sin_addr) >= 0)
        {
            if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) >= 0)
            {
                return sockfd;
            }
        }
    }

    return -1;
}

/**
 * Writes an entire buffer to the socket.
 * Returns the number of bytes written or -1 upon failure.
 */
int util_write_buffer_to_socket(const Buffer *t_buffer, int t_socket)
{
    size_t data_written = 0;
    while (data_written < t_buffer->length)
    {
        size_t written_this_iteration = 0;
        written_this_iteration += write(t_socket,
                                        t_buffer->data + data_written,
                                        t_buffer->length - data_written);

        // check for errors
        if (written_this_iteration == -1)
        {
            return -1;
        }

        data_written += written_this_iteration;
    }

    return data_written;
}

/**
 * Reads the socket, storing its contents inside the buffer. The buffer will be
 * reallocated until all the data is read.
 * Returns 0 on success or -1 upon failure.
 */
int util_read_buffer_from_socket(Buffer *t_buffer, int t_socket)
{
    size_t data_read = 0, data_read_this_iteration;

    while (true)
    {
        data_read_this_iteration = read(t_socket,
                                        t_buffer->data,
                                        t_buffer->length - data_read);

        // check if an error occurred
        if (data_read_this_iteration == -1)
        {
            return -1;
        }
        else
        {
            // check if we are finished reading data
            if (data_read_this_iteration == 0)
            {
                break;
            }
            else
            {
                // we have more data to read
                data_read += data_read_this_iteration;

                // check if we need to reallocate more space for the response
                if (t_buffer->length - data_read == 0)
                {

                    // allocate more space and check for errors
                    if (buffer_double_size(t_buffer) == -1)
                    {
                        return -1;
                    }
                }
            }
        }
    }

    return data_read;
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
    Buffer *res_buf = NULL, *req_buf = NULL;
    int socket = 0;

    // attempt to create the response buffer
    if ((res_buf = buffer_create(BUF_SIZE)) != NULL)
    {
        // attempt to create the request buffer
        if ((req_buf = util_create_request((const char *)page)) != NULL)
        {
            // attempt to create the socket
            if ((socket = util_create_socket(host, port)) != -1)
            {
                // attempt to send the buffer down the socket
                if (util_write_buffer_to_socket(req_buf, socket) != -1)
                {
                    printf("Data sent ok\n");

                    // attempt to read the data into the buffer
                    if (util_read_buffer_from_socket(res_buf, socket) != -1)
                    {
                        printf("Data received ok\n%s\n", res_buf->data);
                    }
                }

                // close the socket
                close(socket);
            }

            // free the request buffer
            buffer_free(req_buf);
        }
    }

    return res_buf;
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

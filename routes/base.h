#ifndef HIGHLOAD_BASE_H
#define HIGHLOAD_BASE_H

#include <unistd.h>
#include <sstream>
#include <utility>
#include <vector>
#include <sys/epoll.h>
#include <iostream>
#include <functional>
#include <netinet/in.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <cerrno>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <string>

using namespace std;

#define M_RESPONSE_404 "HTTP/1.1 404 Not Found\nContent-Type: application/json\nContent-Length: 2\n\n{}"
#define M_RESPONSE_400 "HTTP/1.1 400 Bad Request\nContent-Type: application/json\nContent-Length: 2\n\n{}"
#define M_RESPONSE_200_PREFIX "HTTP/1.1 200 OK\nContent-Type: application/json\nContent-Length: "

#define M_RESPONSE_MAX_SIZE 100000

class RouteProcessor
{
protected:
    static size_t _dec_length(size_t n)
    {
        size_t r = 0;
        while (n > 0)
        {
            n /= 10;
            r++;
        }
        return r;
    }

public:
    int socket_fd;

    explicit RouteProcessor(int socket_fd)
    {
        this->socket_fd = socket_fd;
    }

    void handle_404()
    {
        if (send(socket_fd, M_RESPONSE_404, M_STRLEN(M_RESPONSE_404), 0) <= 0)
        {
            perror("404 write()");
            abort();
        }
    }

    void handle_400()
    {
        if (send(socket_fd, M_RESPONSE_400, M_STRLEN(M_RESPONSE_400), 0) <= 0)
        {
            perror("400 write()");
            abort();
        }
    }

    void handle_200()
    {
        if (send(socket_fd, M_RESPONSE_200_PREFIX "2\n\n{}", M_STRLEN(M_RESPONSE_400 "2\n\n{}"), 0) <= 0)
        {
            perror("200 write()");
            abort();
        }
    }

    void handle_200(const char* body, size_t content_length)
    {
        static thread_local char* headers = NULL;
        int prefix_length = M_STRLEN(M_RESPONSE_200_PREFIX);
        if (headers == NULL)
        {
            headers = (char*) malloc(prefix_length + M_RESPONSE_MAX_SIZE);
            strcpy(headers, M_RESPONSE_200_PREFIX);
        }

        size_t additional_buf_length = _dec_length(content_length) + 2;
        sprintf(headers + prefix_length, "%d\n\n", (int) content_length);

        memcpy(headers + prefix_length + additional_buf_length, body, content_length);

        if (send(socket_fd, headers, prefix_length + additional_buf_length + content_length, 0) <= 0)
        {
            perror("200 all write()");
            abort();
        }
    }

    void handle(const char* all, size_t length)
    {
        if (send(socket_fd, all, length, 0) <= 0)
        {
            perror("handle write()");
            abort();
        }
    }
};


#endif //HIGHLOAD_BASE_H

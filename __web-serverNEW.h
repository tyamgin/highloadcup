/**
 * Based on https://banu.com/blog/2/how-to-use-epoll-a-complete-example-in-c/
 * https://stackoverflow.com/questions/6954489/c-whats-the-way-to-make-a-poolthread-with-pthreads - thread pool
 */

#ifndef HIGHLOAD_WEB_SERVER_H
#define HIGHLOAD_WEB_SERVER_H

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
#include <queue>

#include "logger.h"
#include "state.h"

using namespace std;


#define M_FLAGS (EPOLLIN | EPOLLET)

#define M_EPOLL_MAX_EVENTS 64
#define M_LISTEN_MAX_CONN SOMAXCONN
#define M_THREADS_COUNT 4

class WebServer;

class WebServerWorker
{
    WebServer *_server = NULL;

public:
    int efd;
    bool is_master;

    explicit WebServerWorker(WebServer *server, bool is_master);

    void do_work();
};


class WebServer
{
    typedef int (*Handler)(int);

    uint16_t _port;
    Handler _handler;
    WebServerWorker* workers[M_THREADS_COUNT];
    int sfd;

    int _create_and_bind()
    {
        addrinfo hints{};
        addrinfo *result, *rp;
        int sfd = -1;

        memset(&hints, 0, sizeof(struct addrinfo));
        hints.ai_family = AF_UNSPEC;     // Return IPv4 and IPv6 choices
        hints.ai_socktype = SOCK_STREAM; // We want a TCP socket
        hints.ai_flags = AI_PASSIVE;     // All interfaces

        char port[6];
        sprintf(port, "%d", (int) this->_port);
        int s = getaddrinfo(NULL, port, &hints, &result);
        if (s != 0)
        {
            M_ERROR("getaddrinfo: " << gai_strerror(s));
            abort();
        }

        for (rp = result; rp != NULL; rp = rp->ai_next)
        {
            sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
            if (sfd == -1)
                continue;

            if (bind(sfd, rp->ai_addr, rp->ai_addrlen) == 0)
                break;

            close(sfd);
        }

        if (rp == NULL)
        {
            perror("Could not bind");
            abort();
        }

        freeaddrinfo(result);

        return sfd;
    }

    static int _make_socket_nodelay(int sfd)
    {
        int flags = 1;
        if (setsockopt(sfd, IPPROTO_TCP, TCP_NODELAY, &flags, sizeof(int)) < 0)
        {
            perror("setsockopt TCP_NODELAY");
            abort();
        }
        if (setsockopt(sfd, IPPROTO_TCP, TCP_QUICKACK, &flags, sizeof(int)) < 0)
        {
            perror("setsockopt TCP_QUICKACK");
            abort();
        }
//        int prior = 6;
//        if (setsockopt(sfd, SOL_SOCKET, SO_PRIORITY, &prior, sizeof(int)) < 0)
//        {
//            perror("setsockopt SO_PRIORITY");
//            abort();
//        }

//        int bfsz = 128;
//        if (setsockopt(sfd, SOL_SOCKET, SO_RCVBUF, &bfsz, sizeof(int)) < 0)
//        {
//            perror("setsockopt SO_RCVBUF");
//            abort();
//        }


//        if (setsockopt(sfd, SOL_SOCKET, SO_DONTROUTE, (char*) &flags, sizeof(int)) < 0)
//        {
//            perror("setsockopt SO_DONTROUTE");
//            abort();
//        }
    }

    static void _make_socket_non_blocking(int sfd)
    {
        int flags = fcntl(sfd, F_GETFL, 0);
        if (flags == -1)
        {
            perror("fcntl F_GETFL");
            abort();
        }

        flags |= O_NONBLOCK;
        if (fcntl(sfd, F_SETFL, flags) == -1)
        {
            perror("fcntl F_SETFL");
            abort();
        }
    }

    static void* _do_work(void* context)
    {
        ((WebServerWorker*) context)->do_work();
        return NULL;
    }

public:
    explicit WebServer(uint16_t port, Handler handler)
    {
        _port = port;
        _handler = handler;
    }

    int start()
    {
        sfd = _create_and_bind();

        _make_socket_non_blocking(sfd);
        _make_socket_nodelay(sfd);

        if (listen(sfd, M_LISTEN_MAX_CONN) == -1)
        {
            perror("listen");
            abort();
        }

        for(int i = 0; i < M_THREADS_COUNT; i++)
        {
            workers[i] = new WebServerWorker(this, i == 0);
        }

        epoll_event event{};
        event.data.fd = sfd;
        event.events = M_FLAGS;

        if (epoll_ctl(workers[0]->efd, EPOLL_CTL_ADD, sfd, &event) == -1)
        {
            perror("epoll_ctl");
            abort();
        }

        pthread_t threads[M_THREADS_COUNT];

        for (int i = 0; i < M_THREADS_COUNT; i++)
            pthread_create(&threads[i], NULL, &WebServer::_do_work, workers[i]);

        for (int i = 0; i < M_THREADS_COUNT; i++)
            pthread_join(threads[i], NULL);

        close(sfd);
    }

    friend class WebServerWorker;
};



WebServerWorker::WebServerWorker(WebServer* server, bool is_master)
{
    this->_server = server;
    this->is_master = is_master;

    efd = epoll_create1(0);

    if (efd == -1)
    {
        perror("epoll_create");
        abort();
    }
}

void WebServerWorker::do_work()
{
    auto sfd = _server->sfd;
    epoll_event event{};
    epoll_event events[M_EPOLL_MAX_EVENTS];

    while (1)
    {
        int n = epoll_wait(efd, events, M_EPOLL_MAX_EVENTS, 0);

        if (n > 0)
            M_DEBUG_LOG((long long) pthread_self() << "| wake ");

        if (n < 0)
        {
            perror("epoll_wait");
            //abort();
        }

        if (n > 1)
            M_DEBUG_LOG((long long) pthread_self() << "| " << n << " in queue");

        for (int i = 0; i < n; i++)
        {
            auto socket_fd = events[i].data.fd;

            M_DEBUG_LOG((long long) pthread_self() << "| sock in: " << socket_fd);


            if ((events[i].events & EPOLLERR) || (events[i].events & EPOLLHUP) || (events[i].events & EPOLLRDHUP) || (!(events[i].events & EPOLLIN)))
            {
                M_ERROR("epoll error: " << events[i].events);
                M_LOG("Query idx: " << state.query_id);
                close(events[i].data.fd);
            }
            else if (sfd == socket_fd)
            {
                assert(is_master);
                while (1)
                {
                    sockaddr in_addr{};
                    socklen_t in_len;
                    in_len = sizeof in_addr;

                    int infd = accept4(sfd, &in_addr, &in_len, SOCK_NONBLOCK);
                    if (infd == -1)
                    {
                        if (errno == EAGAIN || errno == EWOULDBLOCK)
                            break;

                        perror("accept");
                        break;
                    }

                    M_DEBUG_LOG((long long) pthread_self() << "| accepted: " << infd);

                    WebServer::_make_socket_nodelay(infd);

                    event.data.fd = infd;
                    event.events = M_FLAGS;
                    auto target_efd = _server->workers[infd % M_THREADS_COUNT]->efd;
                    M_DEBUG_LOG(infd << " -> " << target_efd);
                    if (epoll_ctl(target_efd, EPOLL_CTL_ADD, infd, &event) == -1)
                    {
                        perror("epoll_ctl add");
                        abort();
                    }
                }
            }
            else
            {
                _server->_handler(socket_fd);
            }
        }
    }
}

#endif //HIGHLOAD_WEB_SERVER_H
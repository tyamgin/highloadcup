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

using namespace std;

#ifndef EPOLLEXCLUSIVE
#define EPOLLEXCLUSIVE (1 << 28)
#endif

#define M_FLAGS (EPOLLIN | EPOLLEXCLUSIVE | EPOLLET)

#define M_EPOLL_MAX_EVENTS 32
#define M_THREADS_COUNT 4

class WebServer
{
    typedef int (*Handler)(int);

    uint16_t _port;
    Handler _handler;
    int sfd, efd;


    static void* _worker_main(void *context)
    {
        ((WebServer*) context)->_worker_main();
        return NULL;
    }

    void _worker_main()
    {
        //pthread_detach(pthread_self());

        int s;
        epoll_event event{};
        epoll_event *events = (epoll_event *) calloc(M_EPOLL_MAX_EVENTS, sizeof event);

        // The event loop
        while (1)
        {

#ifdef DEBUG
            //logger::log(to_string((long long) pthread_self()) + "| waiting");
#endif
            int n = epoll_wait(efd, events, M_EPOLL_MAX_EVENTS, -1);

#ifdef DEBUG
            logger::log(to_string((long long) pthread_self()) + "| wake ");
#endif

            if (n < 0)
            {
                perror("epoll_wait");
                //abort();
            }
#ifdef DEBUG
            //logger::log(to_string(n) + " in queue");
#endif
            for (int i = 0; i < n; i++)
            {
                auto socket_fd = events[i].data.fd;
#ifdef DEBUG
                logger::log(to_string((long long) pthread_self()) + "| sock in: " + to_string(socket_fd));
#endif
                if ((events[i].events & EPOLLERR) || (events[i].events & EPOLLHUP) || (!(events[i].events & EPOLLIN)))
                {
                    // An error has occured on this fd, or the socket is not
                    // ready for reading (why were we notified then?)
                    logger::error("epoll error");
                    close(events[i].data.fd);
                    abort();
                }
                else if (sfd == socket_fd)
                {
                    // We have a notification on the listening socket, which
                    // means one or more incoming connections.
                    while (1)
                    {
                        sockaddr in_addr{};
                        socklen_t in_len;
                        in_len = sizeof in_addr;

                        int infd = accept4(sfd, &in_addr, &in_len, SOCK_NONBLOCK);
                        if (infd == -1)
                        {
                            if (errno == EAGAIN || errno == EWOULDBLOCK)
                            {
                                // We have processed all incoming connections.
                                break;
                            }

                            perror("accept");
                            break;
                        }
#ifdef DEBUG
                        logger::log(to_string((long long) pthread_self()) + "| accepted: " + to_string(infd));
#endif

                        // Make the incoming socket non-blocking and add it to the list of fds to monitor.
                        _make_socket_non_blocking(infd);
                        _make_socket_nodelay(infd);

                        event.data.fd = infd;
                        event.events = M_FLAGS;
                        s = epoll_ctl(efd, EPOLL_CTL_ADD, infd, &event);
                        if (s == -1)
                        {
                            perror("epoll_ctl add");
                            abort();
                        }
                    }
                }
                else
                {
                    // We have data on the fd waiting to be read. Read and
                    // display it. We must read whatever data is available
                    // completely, as we are running in edge-triggered mode
                    // and won't get a notification again for the same
                    // data.


                    _handler(socket_fd);
                }
            }
        }

        free(events);
    }

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
            logger::error((string)"getaddrinfo: " + gai_strerror(s));
            abort();
        }

        for (rp = result; rp != NULL; rp = rp->ai_next)
        {
            sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
            if (sfd == -1)
                continue;

            s = bind(sfd, rp->ai_addr, rp->ai_addrlen);
            if (s == 0)
            {
                // We managed to bind successfully!
                break;
            }

            close(sfd);
        }

        if (rp == NULL)
        {
            logger::log("Could not bind");
            abort();
        }

        freeaddrinfo(result);

        return sfd;
    }

    static int _make_socket_nodelay(int sfd)
    {
        int flags = 1;
        int s = setsockopt(sfd, IPPROTO_TCP, TCP_NODELAY, (char*) &flags, sizeof(int));
        if (s < 0)
        {
            perror("setsockopt");
            abort();
        }
    }

    static void _make_socket_non_blocking(int sfd)
    {
        int flags, s;

        flags = fcntl(sfd, F_GETFL, 0);
        if (flags == -1)
        {
            perror("fcntl F_GETFL");
            abort();
        }

        flags |= O_NONBLOCK;
        s = fcntl(sfd, F_SETFL, flags);
        if (s == -1)
        {
            perror("fcntl F_SETFL");
            abort();
        }
    }

public:
    explicit WebServer(uint16_t port, Handler handler)
    {
        _port = port;
        _handler = handler;
    }

    int start()
    {
        int s;

        sfd = _create_and_bind();
        if (sfd == -1)
        {
            logger::error("sfd == -1");
            abort();
        }

        _make_socket_non_blocking(sfd);
        _make_socket_nodelay(sfd);

        s = listen(sfd, SOMAXCONN);
        if (s == -1)
        {
            perror("listen");
            abort();
        }

        //efd = epoll_create(M_EPOLL_MAX_EVENTS);
        efd = epoll_create1(0);

        if (efd == -1)
        {
            perror("epoll_create");
            abort();
        }


        epoll_event event{};
        event.data.fd = sfd;
        event.events = M_FLAGS;
        s = epoll_ctl(efd, EPOLL_CTL_ADD, sfd, &event);
        if (s == -1)
        {
            perror("epoll_ctl");
            abort();
        }

        // Buffer where events are returned

#if M_THREADS_COUNT
        pthread_t threads[M_THREADS_COUNT];

        for (int i = 0; i < M_THREADS_COUNT; i++)
            pthread_create(&threads[i], NULL, _worker_main, this);

        for (int i = 0; i < M_THREADS_COUNT; i++)
            pthread_join(threads[i], NULL);
#else
        _worker_main(this);
#endif
        //close(sfd);
    }
};

#endif //HIGHLOAD_WEB_SERVER_H
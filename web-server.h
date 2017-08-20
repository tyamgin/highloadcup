/**
 * Based on https://banu.com/blog/2/how-to-use-epoll-a-complete-example-in-c/
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
#include <errno.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>

#include "logger.h"

using namespace std;

#define M_EPOLL_MAX_EVENTS 1024

class WebServer
{
    typedef int (*Handler)(int);

    uint16_t _port;
    Handler _handler;

public:
    explicit WebServer(uint16_t port, Handler handler)
    {
        _port = port;
        _handler = handler;
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

    int start()
    {
        int sfd, s;
        int efd;
        epoll_event event{};
        epoll_event *events;

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

        efd = epoll_create1(0);
        if (efd == -1)
        {
            perror("epoll_create");
            abort();
        }

        event.data.fd = sfd;
        event.events = EPOLLIN | EPOLLET;
        s = epoll_ctl(efd, EPOLL_CTL_ADD, sfd, &event);
        if (s == -1)
        {
            perror("epoll_ctl");
            abort();
        }

        // Buffer where events are returned
        events = (epoll_event*) calloc(M_EPOLL_MAX_EVENTS, sizeof event);

        // The event loop
        while (1)
        {
            int n, i;

#ifdef DEBUG
            logger::log("waiting");
#endif
            n = epoll_wait(efd, events, M_EPOLL_MAX_EVENTS, -1);
#ifdef DEBUG
            logger::log(to_string(n) + " in queue");
#endif
            for (i = 0; i < n; i++)
            {
                if ((events[i].events & EPOLLERR) || (events[i].events & EPOLLHUP) || (!(events[i].events & EPOLLIN)))
                {
                    // An error has occured on this fd, or the socket is not
                    // ready for reading (why were we notified then?)
                    logger::error("epoll error");
                    close(events[i].data.fd);
                    abort();
                }

                else if (sfd == events[i].data.fd)
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

                        // Make the incoming socket non-blocking and add it to the list of fds to monitor.
                        _make_socket_non_blocking(infd);
                        _make_socket_nodelay(infd);

                        event.data.fd = infd;
                        event.events = EPOLLIN | EPOLLET;
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
                    int r = _handler(events[i].data.fd);
//                    if (r < 0)
//                    {
//                        logger::log("cl1");
//                        close(events[i].data.fd);
//                        continue;
//                    }
//                    close(events[i].data.fd);


//                    static thread_local char buf[1 << 10];
//                    while (1)
//                    {
//                        s = read(events[i].data.fd, buf, sizeof(buf));
//                        if (s == 0)
//                        {
//                            //logger::log("read s 0");
//                            //abort();
//                            logger::log("cl2");
//                            close(events[i].data.fd);
//                            break;
//                        }
//                        if (s == -1)
//                        {
//                            if (errno == EAGAIN)
//                            {
//                                close(events[i].data.fd);
//                                break;
//                            }
//                            logger::log("read 0");
//                            abort();
//                        }
//                    }
                }
            }
        }

        free(events);
        close(sfd);
        return EXIT_SUCCESS;
    }
};

#endif //HIGHLOAD_WEB_SERVER_H

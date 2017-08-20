#ifndef HIGHLOAD_WEB_SERVER_H
#define HIGHLOAD_WEB_SERVER_H

#define MAX_EVENTS 32

#include <unistd.h>
#include <sstream>
#include <utility>
#include <vector>
#include <sys/epoll.h>
#include <iostream>
#include <functional>
#include <netinet/in.h>

#include "logger.h"

using namespace std;

class WebServer
{
    uint16_t _port;
    function<int(int)> _handler;

public:
    explicit WebServer(uint16_t port, function<int(int)> handler)
    {
        _port = port;
        _handler = std::move(handler);
    }

    void start()
    {
        // PF_INET
        int master_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

        if (master_fd < 0)
        {
            perror("socket() error");
            exit(EXIT_FAILURE);
        }

        sockaddr_in master_addr{};

        master_addr.sin_family = AF_INET;
        master_addr.sin_port = htons(_port);
        master_addr.sin_addr.s_addr = htonl(INADDR_ANY);

        //master_addr.sin_addr = ws_opts.server_ip; // ???

        if (bind(master_fd, (sockaddr*) &master_addr, sizeof(master_addr)) < 0)
        {
            perror("bind()");
            exit(EXIT_FAILURE);
        }

        //set_nonblock(master_fd); // ???

        if (listen(master_fd, 10000/*SOMAXCONN=128*/) < 0)
        {
            perror("listen()");
            exit(EXIT_FAILURE);
        }

        int epl = epoll_create1(0);
        epoll_event ev{};
        ev.data.fd = master_fd;
        ev.events = EPOLLIN;
        epoll_ctl(epl, EPOLL_CTL_ADD, master_fd, &ev);

        epoll_event evnts[MAX_EVENTS];

        logger::log("Wait for queries");

        while (true)
        {
            int evcnt = epoll_wait(epl, evnts, MAX_EVENTS, -1);

            for (int i = 0; i < evcnt; ++i) {
                if (evnts[i].data.fd == master_fd) {
                    int sock = accept(master_fd, NULL, NULL);
                    //set_nonblock(sock);
                    ev.data.fd = sock;
                    ev.events = EPOLLIN;
                    epoll_ctl(epl, EPOLL_CTL_ADD, sock, &ev);
                    continue;
                }

                epoll_ctl(epl, EPOLL_CTL_DEL, evnts[i].data.fd, NULL);
                if ((evnts[i].events & EPOLLERR) || (evnts[i].events & EPOLLHUP)) {
                    shutdown(evnts[i].data.fd, SHUT_RDWR);
                    close(evnts[i].data.fd);
                } else if (evnts[i].events & EPOLLIN) {
                    _handler(evnts[i].data.fd);
                }
            }
        }
    }
};

#endif //HIGHLOAD_WEB_SERVER_H

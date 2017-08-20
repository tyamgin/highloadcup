#define THREADS_COUNT 0

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sstream>
#include <vector>
#include <iostream>
#include <codecvt>

#include "json.h"
#include "web-server.h"
#include "routes/get_entity.h"
#include "routes/get_user_visits.h"
#include "routes/get_location_avg.h"
#include "routes/post_entity.h"
#include "logger.h"
#include "thread_pool.h"

using namespace std;

#define BUFFER_MAX_SIZE (1 << 17)

ThreadPool thread_pool(THREADS_COUNT);

int handle_request1(int socket_fd)
{
    ssize_t n;
    string response;

    static thread_local char buffer[BUFFER_MAX_SIZE];

    n = recv(socket_fd, buffer, BUFFER_MAX_SIZE, 0);
    if (n < 0)
    {
        perror("recv()");
        return 0;
    }
    if (n == BUFFER_MAX_SIZE)
    {
        logger::error("Buffer max length reached");
        exit(1);
    }
    buffer[n] = 0;

//    logger::log("--- Buffer content ---");
//    logger::log(buffer);
//    logger::log("--- Buffer content end ---");

    bool is_get = buffer[0] == 'G';
    string path;
    for (int i = 4 + !is_get; !isspace(buffer[i]); i++)
        path += buffer[i];

    map<string, string> query;
    if (path.find('?') != string::npos)
    {
        auto parts = Utility::split(path, '?');
        path = parts[0];
        auto quieries = Utility::split(parts[1], '&');
        for (auto &pair : quieries)
        {
            if (!pair.empty())
            {
                auto p = Utility::split(pair, '=');
                query[p[0]] = p.size() > 1 ? Utility::urlDecode(p[1]) : "";
            }
        }
    }

    if (is_get)
    {
        state.gets_count++;
        logger::log(state.gets_count);
        if (Utility::startsWith(path, "/users/"))
        {
            if (Utility::endsWith(path, "/visits"))
            {
                auto user_id = path.substr(strlen("/users/"));
                user_id.resize((int) user_id.size() - strlen("/visits"));
                auto processor = GetUserVisitsRouteProcessor();
                processor.process(user_id.c_str(), query["fromDate"].c_str(), query["toDate"].c_str(),
                                  query["country"].c_str(), query["toDistance"].c_str());
                response = processor.get_http_result();
            }
            else
            {
                auto user_id = path.substr(strlen("/users/"));
                auto processor = GetEntityRouteProcessor();
                processor.process("users", user_id.c_str());
                response = processor.get_http_result();
            }
        }
        else if (Utility::startsWith(path, "/locations/"))
        {
            if (Utility::endsWith(path, "/avg"))
            {
                auto location_id = path.substr(strlen("/locations/"));
                location_id.resize((int) location_id.size() - strlen("/avg"));
                auto processor = GetLocationAvgRouteProcessor();
                processor.process(location_id.c_str(), query["fromDate"].c_str(), query["toDate"].c_str(),
                                  query["fromAge"].c_str(), query["toAge"].c_str(), query["gender"].c_str());
                response = processor.get_http_result();
            }
            else
            {
                auto location_id = path.substr(strlen("/locations/"));
                auto processor = GetEntityRouteProcessor();
                processor.process("locations", location_id.c_str());
                response = processor.get_http_result();
            }
        }
        else if (Utility::startsWith(path, "/visits/"))
        {
            auto visit_id = path.substr(strlen("/visits/"));
            auto processor = GetEntityRouteProcessor();
            processor.process("visits", visit_id.c_str());
            response = processor.get_http_result();
        }
        logger::log(state.gets_count);
    }
    else
    {
        int body_start_pos = 0;
        for (int i = 3; i < n; i++)
        {
            if (buffer[i] == '\n' && buffer[i - 1] == '\n' ||
                buffer[i] == '\n' && buffer[i - 1] == '\r' && buffer[i - 2] == '\n' && buffer[i - 3] == '\r')
            {
                body_start_pos = i + 1;
                break;
            }
        }

        for (auto &entity_name : vector<string> {"users", "locations", "visits"})
        {
            if (Utility::startsWith(path, "/" + entity_name + "/"))
            {
                auto processor = PostEntityRouteProcessor();
                auto entity_id = path.substr(entity_name.size() + 2);
                processor.process(entity_name.c_str(), entity_id.c_str(), buffer + body_start_pos);
                response = processor.get_http_result();
            }
        }
    }


    if (send(socket_fd, response.c_str(), response.size(), 0) < 0)
    {
        perror("write()");
        return 0;
    }

    close(socket_fd);
    return 0;
}

int handle_request(int cliefd)
{
    if (THREADS_COUNT == 0)
        return handle_request1(cliefd);

    thread_pool.add_job([cliefd]() {
        handle_request1(cliefd);
    });
}

int main(int argc, const char *argv[])
{
    logger::log("Unpacking data...");
    state.fill_from_file("/tmp/data/data.zip");
    logger::log("Data unpacked");
#ifdef DEBUG
    const int port = 8080;
#else
    const int port = 80;
#endif
    logger::log("Starting thread pool...");
    thread_pool.start();


    logger::log("Initializing server...");
    WebServer server(port, handle_request);
    logger::log("Starting server...");
    server.start();
    logger::log("Server stopped...");

    thread_pool.join();
    return 0;
}
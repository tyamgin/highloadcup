/**
 * TODO:
 * -
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sstream>

#include "web-server.h"
#include "routes/get_entity.h"
#include "routes/get_user_visits.h"
#include "routes/get_location_avg.h"
#include "routes/post_entity.h"

using namespace std;

#define M_BUFFER_MAX_SIZE (1 << 14)

ssize_t read_buf(int sfd, char* buf, size_t size)
{
    auto n = recv(sfd, buf, size, 0);
    if (n == -1)
    {
        // If errno == EAGAIN, that means we have read all data. So go back to the main loop.
        if (errno != EAGAIN)
        {
            perror("read");
            //abort();
        }
        return -1;
    }
    else if (n == 0)
    {
#ifdef DEBUG
        perror("read() = 0");
#endif
        return 0;
    }
    return n;
}

int handle_request(int socket_fd)
{
    ssize_t n;
    static thread_local char buffer[M_BUFFER_MAX_SIZE];

    n = read_buf(socket_fd, buffer, M_BUFFER_MAX_SIZE);
    if (n <= 0)
    {
        if (n == 0)
        {
            M_DEBUG_LOG((long long) pthread_self() << "| close sock=" << socket_fd);
            close(socket_fd);
        }
        else
        {
            M_LOG((long long) pthread_self() << "| miss sock=" << socket_fd);
        }
        return -1;
    }

    buffer[n] = 0;

    M_DEBUG_LOG((long long) pthread_self() << "| Processing " << state.query_id << " query, sock=" << socket_fd);

    bool is_get = buffer[0] == 'G';

    char* path = buffer + 4 + !is_get;
    char* ptr = path;

    ptr++; // skip slash
    char *id_ptr = NULL;
    EntityType entity_type;
    if (*ptr == 'u')
    {
        entity_type = UserEntity;
        ptr += M_STRLEN("users/");
    }
    else if (*ptr == 'v')
    {
        entity_type = VisitEntity;
        ptr += M_STRLEN("visits/");
    }
    else
    {
        entity_type = LocationEntity;
        ptr += M_STRLEN("locations/");
    }

    id_ptr = ptr;
    while (*ptr != 0 && *ptr != '/' && *ptr != ' ' && *ptr != '?')
        ptr++;

    if (ptr - id_ptr > INT_MAX_LENGTH) // too large id, invalid int
    {
        RouteProcessor(socket_fd).handle_404();
        goto exit;
    }

    if (is_get)
    {
        char *from_date = NULL;
        char *to_date = NULL;
        char *country = NULL;
        char *to_distance = NULL;
        char *from_age = NULL;
        char *to_age = NULL;
        char *gender = NULL;

        bool is_locations_avg = false;
        bool is_user_visits = false;

        if (*ptr == '/')
        {
            *ptr = 0;
            ptr++; // skip slash
            if (*ptr == 'a')
            {
                is_locations_avg = true;
                ptr += M_STRLEN("avg");
            }
            else
            {
                is_user_visits = true;
                ptr += M_STRLEN("visits");
            }
        }

        if (*ptr == '?')
        {
            while (*ptr == '?' || *ptr == '&')
            {
                *ptr = 0;
                ptr++;

#define PARSE_GETS_1(prop, str_prop) { ptr += M_STRLEN(str_prop) + 1; (prop) = ptr; while (*ptr != '&' && *ptr != ' ') ptr++; }

                if (ptr[0] == 'g')      PARSE_GETS_1(gender, "gender")
                else if (ptr[0] == 'c') PARSE_GETS_1(country, "country")
                else if (ptr[2] == 'A') PARSE_GETS_1(to_age, "toAge")
                else if (ptr[4] == 'D') PARSE_GETS_1(from_date, "fromDate")
                else if (ptr[4] == 'A') PARSE_GETS_1(from_age, "fromAge")
                else if (ptr[3] == 'a') PARSE_GETS_1(to_date, "toDate")
                else if (ptr[3] == 'i') PARSE_GETS_1(to_distance, "toDistance")
            }
        }
        *ptr = 0;

        if (entity_type == UserEntity)
        {
            if (is_user_visits)
            {
                auto processor = GetUserVisitsRouteProcessor(socket_fd);
                processor.process(id_ptr, from_date, to_date, country, to_distance);
            }
            else
            {
                auto processor = GetEntityRouteProcessor(socket_fd);
                processor.process(entity_type, id_ptr);
            }
        }
        else if (entity_type == LocationEntity)
        {
            if (is_locations_avg)
            {
                auto processor = GetLocationAvgRouteProcessor(socket_fd);
                processor.process(id_ptr, from_date, to_date, from_age, to_age, gender);
            }
            else
            {
                auto processor = GetEntityRouteProcessor(socket_fd);
                processor.process(LocationEntity, id_ptr);
            }
        }
        else
        {
            auto processor = GetEntityRouteProcessor(socket_fd);
            processor.process(VisitEntity, id_ptr);
        }
    }
    else
    {
        *ptr = 0;
        while (!(ptr[0] == '\n' && ptr[-1] == '\n' || ptr[0] == '\n' && ptr[-1] == '\r' && ptr[-2] == '\n' && ptr[-3] == '\r'))
            ptr++;
        char* body_ptr = ptr + 1;

        auto processor = PostEntityRouteProcessor(socket_fd);
        processor.process(entity_type, id_ptr, body_ptr);
    }
exit:

    if (!is_get)
    {
        //shutdown(socket_fd, SHUT_RDWR);
        close(socket_fd);
    }

    state.query_id++;
    return 0;
}

int main(int argc, const char *argv[])
{
    M_LOG("Unpacking data...");
    state.fill_from_file("/tmp/data");
    M_LOG("Data unpacked");

    const int port = 80;

    M_LOG("Initializing server...");
    WebServer server(port, handle_request);
    M_LOG("Starting server...");
    server.start();
    M_LOG("Server stopped...");

    return 0;
}
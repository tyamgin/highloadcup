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

#define INT_MAX_LENGTH 11
#define M_BUFFER_MAX_SIZE (1 << 12)


ssize_t read_buf(int sfd, char* buf, size_t size)
{
    ssize_t n;
    while (1)
    {
        n = read(sfd, buf, size);
        if (n == -1)
        {
            // If errno == EAGAIN, that means we have read all data. So go back to the main loop.
            if (errno != EAGAIN)
            {
                perror("read");
                abort();
            }
            // it's ok in generally, but abort anyway
            //logger::log("n == -1");
            //abort();
            return -1;
        }
        else if (n == 0)
        {
            //logger::log("n == 0");
            //abort();
            return -1;
            // End of file. The remote has closed the connection.
        }
        break;
    }
    return n;
}


int handle_request(int socket_fd)
{
    ssize_t n;
    static thread_local char buffer[M_BUFFER_MAX_SIZE];

    static int query_id = 1;

    n = read_buf(socket_fd, buffer, M_BUFFER_MAX_SIZE);
    if (n <= 0)
        return -1;

    buffer[n] = 0;

#ifdef DEBUG
    logger::log(to_string((long long) pthread_self()) + "| Processing " + to_string(query_id) + " query, sock=" + to_string(socket_fd));
//    if (socket_fd == 5)
//        sleep(20);
#endif


    bool is_get = buffer[0] == 'G';

    char* path = buffer + 4 + !is_get;
    char* ptr = path;

    ptr++; // skip slash
    char *id_ptr = NULL;
    EntityType entity_type;
    if (*ptr == 'u')
    {
        entity_type = UserEntity;
        ptr += strlen("users/");
    }
    else if (*ptr == 'v')
    {
        entity_type = VisitEntity;
        ptr += strlen("visits/");
    }
    else
    {
        entity_type = LocationEntity;
        ptr += strlen("locations/");
    }

    id_ptr = ptr;
    while (*ptr != 0 && *ptr != '/' && *ptr != ' ' && *ptr != '?')
        ptr++;

    if (ptr - id_ptr > INT_MAX_LENGTH) // too large id, invalid int
    {
        RouteProcessor(socket_fd).handle_404();
        return 0;
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
                ptr += strlen("avg");
            }
            else
            {
                is_user_visits = true;
                ptr += strlen("visits");
            }
        }

        if (*ptr == '?')
        {
            while (*ptr == '?' || *ptr == '&')
            {
                *ptr = 0;
                ptr++;

#define PARSE_GETS_1(prop, str_prop) { ptr += strlen(str_prop) + 1; prop = ptr; while (*ptr != '&' && *ptr != ' ') ptr++; }

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

    if (!is_get)
        close(socket_fd);
    //logger::log(query_id);
    query_id++;
    return 0;
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

    logger::log("Initializing server...");
    WebServer server(port, handle_request);
    logger::log("Starting server...");
    server.start();
    logger::log("Server stopped...");

    //thread_pool.join();
    return 0;
}
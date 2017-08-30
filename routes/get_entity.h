#ifndef HIGHLOAD_GET_LOCATION_H
#define HIGHLOAD_GET_LOCATION_H

#include <string>

#include "../state.h"
#include "base.h"

using namespace std;

class GetEntityRouteProcessor : public RouteProcessor
{
public:
    explicit GetEntityRouteProcessor(int socket_fd) : RouteProcessor(socket_fd)
    {
    }

    void process(EntityType entity_type, const char *str_id)
    {
        if (!Utility::is_int(str_id))
        {
            handle_404();
            return;
        }
        int id = atoi(str_id);

        if (!state.has_entity(entity_type, id))
        {
            handle_404();
            return;
        }

        char *response = state.get_entity(entity_type, id)->response_cache;
        handle(response, strlen(response));
    }
};

#endif //HIGHLOAD_GET_LOCATION_H

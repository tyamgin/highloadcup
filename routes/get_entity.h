#ifndef HIGHLOAD_GET_LOCATION_H
#define HIGHLOAD_GET_LOCATION_H

#include <string>

#include "../state.h"
#include "base.h"

using namespace std;

class GetEntityRouteProcessor : public RouteProcessor
{
public:
    void process(const char* entity_name, const char* str_id)
    {
        if (!Utility::is_int(str_id))
        {
            handle_404();
            return;
        }
        int id = atoi(str_id);
        EntityType entity_type = entity_name[0] == 'u' ? UserEntity : entity_name[0] == 'l' ? LocationEntity : VisitEntity;

        if (!state.has_entity(entity_type, id))
        {
            handle_404();
            return;
        }

        body += state.get_entity(entity_type, id)->to_json();
    }
};

#endif //HIGHLOAD_GET_LOCATION_H

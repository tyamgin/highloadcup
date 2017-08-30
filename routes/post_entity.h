#ifndef HIGHLOAD_POST_USER_NEW_H
#define HIGHLOAD_POST_USER_NEW_H

#include "../state.h"
#include "base.h"
#include "../utility.h"

using namespace std;

class PostEntityRouteProcessor : public RouteProcessor
{
public:
    explicit PostEntityRouteProcessor(int socket_fd) : RouteProcessor(socket_fd)
    {
    }

    void process(EntityType entity_type, const char* str_id, const char* request_body)
    {
        int entity_id = 0;
        if (strcmp(str_id, "new") != 0 && !Utility::tryParseInt(str_id, entity_id))
        {
            handle_404();
            return;
        }

        shared_ptr<Entity> entity;

        if (str_id[0] == 'n')
        {
            if (state.has_entity(entity_type, entity_id)) // possibly not need check (sat says)
            {
                handle_400();
                return;
            }
            entity = EntityFactory::create(entity_type);
        }
        else
        {
            entity = shared_ptr<Entity>(state.get_entity(entity_type, entity_id, true));
            if (entity == NULL)
            {
                handle_404();
                return;
            }
        }

        json::Object json;
        if (!json.parse_simple_object(request_body, (int) strlen(request_body)) || !entity->parse(json, str_id[0] == 'n'))
        {
            handle_400();
            return;
        }

        if (str_id[0] == 'n')
        {
            state.add_entity(entity.get());
        }
        else
        {
            state.update_entity(entity.get());
        }
        handle_200();
    }
};

#endif //HIGHLOAD_POST_USER_NEW_H

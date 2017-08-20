#ifndef HIGHLOAD_POST_USER_NEW_H
#define HIGHLOAD_POST_USER_NEW_H

#include <string>

#include "../state.h"
#include "base.h"
#include "../utility.h"

using namespace std;

class PostEntityRouteProcessor : public RouteProcessor
{
public:
    void process(const char* entity_name, const char* str_id, const char* request_body)
    {
        json::Object json;
        try
        {
            json.parse_simple_object(request_body, (int) strlen(request_body));
        }
        catch(const exception &e)
        {
            handle_400();
            return;
        }
        EntityType entity_type = entity_name[0] == 'u' ? UserEntity : entity_name[0] == 'l' ? LocationEntity : VisitEntity;

        if (strcmp(str_id, "new") != 0 && !Utility::is_int(str_id))
        {
            handle_404();
            return;
        }

        int entity_id = str_id[0] == 'n' ? 0 : atoi(str_id);
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

        if (!entity->parse(json, str_id[0] == 'n'))
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
        body += "{}";
    }
};

#endif //HIGHLOAD_POST_USER_NEW_H

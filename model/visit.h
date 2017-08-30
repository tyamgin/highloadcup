#ifndef HIGHLOAD_VISIT_H
#define HIGHLOAD_VISIT_H


#include <sys/types.h>

#include "entity.h"

using namespace std;

class Visit : public Entity
{
public:
    int id;
    int location;
    int user;
    int visited_at;
    int8_t mark;

    bool parse(const json::Object &json_obj, bool require_all) override
    {
        int props_count = 0;
        for (auto &pair : json_obj.properties)
        {
            const char* key = pair.first;
            const json::Value &val = pair.second;

            TRY_PARSE_INT_VAL(id)
            TRY_PARSE_INT_VAL(location)
            TRY_PARSE_INT_VAL(user)
            TRY_PARSE_INT_VAL(visited_at)
            TRY_PARSE_INT_VAL(mark)
        }
        if (mark < 0 || mark > 5)
            return false;
        if (require_all && props_count != 5)
            return false;

        static thread_local char buf[512] = {};
        sprintf(buf, "{\"id\":%d,\"location\":%d,\"user\":%d,\"visited_at\":%d,\"mark\":%d}", id, location, user, visited_at, (int) mark);
        TRY_PARSE_COPY_JSON(response_cache, buf)

        return true;
    }
};


#endif //HIGHLOAD_VISIT_H

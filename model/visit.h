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

    string to_json() const override
    {
        return (string) "{\"id\":" + to_string(id) + ",\"location\":" + to_string(location) + ",\"user\":" +
               to_string(user) + ",\"visited_at\":" + to_string(visited_at) + ",\"mark\":" + to_string(mark) + '}';
    }

    bool parse(const json::Object &json_obj, bool require_all) override
    {
        int props_count = 0;
        for (auto &pair : json_obj.properties)
        {
            auto &key = pair.first;
            auto &val = pair.second;

            TRY_PARSE_INT_VAL(id)
            TRY_PARSE_INT_VAL(location)
            TRY_PARSE_INT_VAL(user)
            TRY_PARSE_INT_VAL(visited_at)
            TRY_PARSE_INT_VAL(mark)
            if (mark < 0 || mark > 5)
                return false;
        }
        if (require_all && props_count != 5)
            return false;

        return true;
    }
};


#endif //HIGHLOAD_VISIT_H

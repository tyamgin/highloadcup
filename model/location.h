#ifndef HIGHLOAD_LOCATION_H
#define HIGHLOAD_LOCATION_H

#include <string>

#include "../json.h"
#include "entity.h"

using namespace std;

class Location : public Entity
{
public:
    int id;
    char* place;
    char* country;
    char* city;
    int distance;

    bool parse(const json::Object &json_obj, bool require_all) override
    {
        int props_count = 0;
        for (auto &pair : json_obj.properties)
        {
            char* key = pair.first;
            const json::Value &val = pair.second;

            TRY_PARSE_INT_VAL(id)
            TRY_PARSE_STR_VAL(place)
            TRY_PARSE_STR_VAL(country)
            TRY_PARSE_STR_VAL(city)
            TRY_PARSE_INT_VAL(distance)
        }

        if (require_all && props_count != 5)
            return false;

        _fix_unicode(country);

        static thread_local char buf[512] = {};
        sprintf(buf, "{\"id\":%d,\"place\":\"%s\",\"country\":\"%s\",\"city\":\"%s\",\"distance\":%d}", id, place, country, city, distance);
        TRY_PARSE_COPY_JSON(json, buf)

        return true;
    }
};


#endif //HIGHLOAD_LOCATION_H

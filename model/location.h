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
    HashType country_hash;
    int distance;

    bool parse(const json::Object &json_obj, bool require_all) override
    {
        int props_count = 0;
        auto prev_country = country;

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

        if (country != prev_country)
        {
            if (country[2] == 208 && country[3] == 168)
                country=country;
            _fix_unicode(country);
            country_hash = 0;
            HashType p = 1;
            for (char *c = country; *c != 0; c++)
                country_hash = country_hash * p + (HashType) static_cast<unsigned char>(*c), p *= M_STRING_HASH_BASE;
        }

        static thread_local char buf[512] = {};
        sprintf(buf, "{\"id\":%d,\"place\":\"%s\",\"country\":\"%s\",\"city\":\"%s\",\"distance\":%d}", id, place, country, city, distance);
        TRY_PARSE_COPY_JSON(json, buf)

        return true;
    }
};


#endif //HIGHLOAD_LOCATION_H

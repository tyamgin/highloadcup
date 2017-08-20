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
    string place;
    string country;
    string city;
    int distance;

    string to_json() const override
    {
        return (string) "{\"id\":" + to_string(id) + ",\"place\":\"" + place + "\",\"country\":\"" + country +
               "\",\"city\":\"" + city + "\",\"distance\":" + to_string(distance) + '}';
    }

    bool parse(const json::Object &json_obj, bool require_all) override
    {
        int props_count = 0;
        for (auto &pair : json_obj.properties)
        {
            auto &key = pair.first;
            auto &val = pair.second;

            TRY_PARSE_INT_VAL(id)
            TRY_PARSE_STR_VAL(place)
            TRY_PARSE_STR_VAL(country)
            TRY_PARSE_STR_VAL(city)
            TRY_PARSE_INT_VAL(distance)
            country = _fix_unicode(country);
        }
        if (require_all && props_count != 5)
            return false;

        return true;
    }
};


#endif //HIGHLOAD_LOCATION_H

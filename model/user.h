#ifndef HIGHLOAD_USER_H
#define HIGHLOAD_USER_H

#include <string>
#include "../json.h"
#include "entity.h"

using namespace std;

class User : public Entity
{
public:
    enum UserGender
    {
        Male,
        Female
    };

    int id;
    string email;
    string first_name;
    string last_name;
    UserGender gender;
    int birth_date;

    string to_json() const override
    {
        return (string) "{\"id\":" + to_string(id) + ",\"email\":\"" + email + "\",\"first_name\":\"" + first_name +
               "\",\"last_name\":\"" + last_name + "\",\"gender\":\"" + (gender == Male ? 'm' : 'f') +
               "\",\"birth_date\":" + to_string(birth_date) + '}';
    }

    bool parse(const json::Object &json_obj, bool require_all) override
    {
        int props_count = 0;
        for (auto &pair : json_obj.properties)
        {
            auto &key = pair.first;
            auto &val = pair.second;

            TRY_PARSE_INT_VAL(id)
            TRY_PARSE_STR_VAL(email)
            TRY_PARSE_STR_VAL(first_name)
            TRY_PARSE_STR_VAL(last_name)
            TRY_PARSE_GENDER_VAL(gender)
            TRY_PARSE_INT_VAL(birth_date)
        }
        if (require_all && props_count != 6)
            return false;

        return true;
    }
};


#endif //HIGHLOAD_USER_H

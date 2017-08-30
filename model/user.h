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
    char* email;
    char* first_name;
    char* last_name;
    UserGender gender;
    int birth_date;


    bool parse(const json::Object &json_obj, bool require_all) override
    {
        int props_count = 0;
        for (auto &pair : json_obj.properties)
        {
            char* key = pair.first;
            const json::Value &val = pair.second;

            TRY_PARSE_INT_VAL(id)
            TRY_PARSE_STR_VAL(email)
            TRY_PARSE_STR_VAL(first_name)
            TRY_PARSE_STR_VAL(last_name)
            TRY_PARSE_GENDER_VAL(gender)
            TRY_PARSE_INT_VAL(birth_date)
        }
        if (require_all && props_count != 6)
            return false;

        static thread_local char buf[512] = {};
        sprintf(buf, "{\"id\":%d,\"email\":\"%s\",\"first_name\":\"%s\",\"last_name\":\"%s\",\"gender\":\"%c\",\"birth_date\":%d}",
                id, email, first_name, last_name, gender == Male ? 'm' : 'f', birth_date);
        TRY_PARSE_COPY_JSON(response_cache, buf)
        
        return true;
    }
};


#endif //HIGHLOAD_USER_H

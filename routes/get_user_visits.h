#ifndef HIGHLOAD_GET_USER_VISITS_H
#define HIGHLOAD_GET_USER_VISITS_H

#include <string>
#include <algorithm>

#include "../state.h"
#include "base.h"
#include "../utility.h"

using namespace std;

struct UserVisitItem
{
    int8_t mark;
    int visited_at;
    const char* place;
};

class GetUserVisitsRouteProcessor : public RouteProcessor
{
public:
    explicit GetUserVisitsRouteProcessor(int socket_fd) : RouteProcessor(socket_fd)
    {
    }

    void process(const char* str_user_id,
                 const char* str_from_date,
                 const char* str_to_date,
                 const char* country,
                 const char* str_to_distance)
    {
        if (!Utility::is_int(str_user_id))
        {
            handle_404();
            return;
        }
        int user_id = atoi(str_user_id);

        if (!state.users.contains(user_id))
        {
            handle_404();
            return;
        }

        if (str_from_date && !Utility::is_int(str_from_date)
            || str_to_date && !Utility::is_int(str_to_date)
            || str_to_distance && !Utility::is_int(str_to_distance))
        {
            handle_400();
            return;
        }

        int from_date = str_from_date == NULL ? INT_MIN : atoi(str_from_date) + 1;
        int to_date = str_to_date == NULL ? INT_MAX : atoi(str_to_date) - 1;
        int to_distance = str_to_distance == NULL ? INT_MAX : atoi(str_to_distance) - 1;

        auto &user = state.users[user_id];
        if (user == NULL)
            throw runtime_error("user is NULL");

        vector<UserVisitItem> visits;

        auto country_hash = country ? Utility::stringUrlDecodedHash(country) : 0;

        for (auto &visit : state.user_visits[user->id])
        {
            if (from_date <= visit->visited_at && visit->visited_at <= to_date)
            {
                auto &location = state.locations[visit->location];

                if (location->distance <= to_distance)
                {
                    if (country == NULL || location->country_hash == country_hash)
                    {
                        visits.push_back({
                             visit->mark,
                             visit->visited_at,
                             location->place
                        });
                    }
                }
            }
        }
        sort(visits.begin(), visits.end(), [](const UserVisitItem &a, const UserVisitItem &b) {
            return a.visited_at < b.visited_at;
        });

        static thread_local char buf[M_RESPONSE_MAX_SIZE] = "{\"visits\":[";
        char* ptr = buf + sizeof("{\"visits\":[") - 1;

        bool first = true;
        for (auto &visit : visits)
        {
            if (!first)
                *ptr++ = ',';
            first = false;

            memcpy(ptr, "{\"mark\":", 8);
            ptr += 8;
            *ptr++ = char(visit.mark + '0');

            memcpy(ptr, ",\"visited_at\":", 14);
            ptr += 14;

            ptr += sprintf(ptr, "%d", visit.visited_at);

            memcpy(ptr, ",\"place\":\"", 10);
            ptr += 10;

            size_t place_len = strlen(visit.place);
            memcpy(ptr, visit.place, place_len);
            ptr += place_len;

            *ptr++ = '"';
            *ptr++ = '}';
        }
        *ptr++ = ']';
        *ptr++ = '}';

        handle_200(buf, ptr - buf);
    }
};

#endif //HIGHLOAD_GET_USER_VISITS_H

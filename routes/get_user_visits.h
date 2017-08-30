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
        int user_id;
        if (!Utility::tryParseInt(str_user_id, user_id))
        {
            handle_404();
            return;
        }

        if (user_id >= M_USERS_MAX_ID || state.users[user_id] == NULL)
        {
            handle_404();
            return;
        }

        int from_date = INT_MIN, to_date = INT_MAX, to_distance = INT_MAX;

        if (str_from_date && !Utility::tryParseInt(str_from_date, from_date)
            || str_to_date && !Utility::tryParseInt(str_to_date, to_date)
            || str_to_distance && !Utility::tryParseInt(str_to_distance, to_distance))
        {
            handle_400();
            return;
        }

        if (from_date != INT_MAX) from_date++;
        if (to_date != INT_MIN) to_date--;
        if (to_distance != INT_MIN) to_distance--;
        // TODO: wrong for visited_at=INT_MIN  and no str_from_date=NULL

        auto &user = state.users[user_id];
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
        char* ptr = buf + M_STRLEN("{\"visits\":[");

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

        if (ptr - buf >= M_RESPONSE_MAX_SIZE)
        {
            M_ERROR("response limit exceeded");
            abort();
        }

        handle_200(buf, ptr - buf);
    }
};

#endif //HIGHLOAD_GET_USER_VISITS_H

#ifndef HIGHLOAD_GET_LOCATION_AVG_H
#define HIGHLOAD_GET_LOCATION_AVG_H

#include <string>

#include "../state.h"
#include "base.h"
#include "../utility.h"

using namespace std;

class GetLocationAvgRouteProcessor : public RouteProcessor
{
public:
    explicit GetLocationAvgRouteProcessor(int socket_fd) : RouteProcessor(socket_fd)
    {
    }

    void process(
            const char* str_id,
            const char* str_from_date,
            const char* str_to_date,
            const char* str_from_age,
            const char* str_to_age,
            const char* str_gender
    )
    {
        int location_id;
        if (!Utility::tryParseInt(str_id, location_id))
        {
            handle_404();
            return;
        }

        if (location_id < 0 || location_id >= M_LOCATIONS_MAX_ID || state.locations[location_id] == NULL)
        {
            handle_404();
            return;
        }

        int from_date = INT_MIN;
        int to_date = INT_MAX;
        int from_age, to_age;

        if (str_from_date && !Utility::tryParseInt(str_from_date, from_date)
            || str_to_date && !Utility::tryParseInt(str_to_date, to_date)
            || str_from_age && !Utility::tryParseInt(str_from_age, from_age)
            || str_to_age && !Utility::tryParseInt(str_to_age, to_age)
            || str_gender && (str_gender[0] != 'm' || str_gender[1] != 0) && (str_gender[0] != 'f' || str_gender[1] != 0)
                )
        {
            handle_400();
            return;
        }

        if (from_date != INT_MAX) from_date++;
        if (to_date != INT_MIN) to_date--;

        int from_birth = str_to_age == NULL ? INT_MIN : (int) min((long long)INT_MAX, (long long)_time_minus_years(*state.cur_time, to_age) + 1);
        int to_birth = str_from_age == NULL ? INT_MAX : (int) max((long long)INT_MIN, (long long)_time_minus_years(*state.cur_time, from_age) - 1);

        int sum = 0;
        int count = 0;
        for(auto &visit : state.location_visits[location_id])
        {
            if (from_date <= visit->visited_at && visit->visited_at <= to_date)
            {
                auto &user = state.users[visit->user];
                if (from_birth <= user->birth_date && user->birth_date <= to_birth)
                {
                    if (str_gender == NULL || str_gender[0] == 'm' && user->gender == User::Male || str_gender[0] == 'f' && user->gender == User::Female)
                    {
                        sum += visit->mark;
                        count++;
                    }
                }
            }
        }

        if (count == 0)
        {
            sum = 0;
            count = 1;
        }

#define M_LOCATION_AVG_RESPONSE_PREFIX M_RESPONSE_200_PREFIX "15\n\n{\"avg\":"
        const int offset = M_STRLEN(M_LOCATION_AVG_RESPONSE_PREFIX);
        static thread_local char buf[offset + 20] = M_LOCATION_AVG_RESPONSE_PREFIX;

        sprintf(buf + offset, "%.5f}", 1.0 * sum / count + 1e-10);
        handle(buf, offset + 8);
    }

private:
    static time_t _time_minus_years(tm time, int years)
    {
        time.tm_year -= years;
        return mktime(&time);
    }
};

#endif //HIGHLOAD_GET_LOCATION_AVG_H

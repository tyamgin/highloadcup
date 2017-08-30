#ifndef HIGHLOAD_DB_H
#define HIGHLOAD_DB_H

#define M_USERS_MAX_ID 1'100'000 // actual size 1000074 + 40000
#define M_LOCATIONS_MAX_ID 900'000 // actual size 761314 + 40000
#define M_VISITS_MAX_ID 11'000'000 // actual size 10000740 + 40000

#include <map>
#include <string>
#include <fstream>
#include <dirent.h>
#include <cstring>
#include <memory>

#include "model/user.h"
#include "model/location.h"
#include "model/visit.h"
#include "logger.h"

using namespace std;

enum EntityType
{
    UserEntity,
    LocationEntity,
    VisitEntity
};

class EntityFactory
{
public:
    static shared_ptr<Entity> create(EntityType type)
    {
        if (type == UserEntity)
            return make_shared<User>();
        if (type == LocationEntity)
            return make_shared<Location>();
        if (type == VisitEntity)
            return make_shared<Visit>();
    }
};

class State
{
public:
    User* users[M_USERS_MAX_ID];
    Location* locations[M_LOCATIONS_MAX_ID];
    Visit* visits[M_VISITS_MAX_ID];
    vector<Visit*> user_visits[M_VISITS_MAX_ID];
    vector<Visit*> location_visits[M_VISITS_MAX_ID];

    bool has_entity(EntityType type, int id)
    {
        return type == UserEntity && id >= 0 && id < M_USERS_MAX_ID && users[id] != NULL
            || type == LocationEntity && id >= 0 && id < M_LOCATIONS_MAX_ID && locations[id] != NULL
            || type == VisitEntity && id >= 0 && id < M_VISITS_MAX_ID && visits[id] != NULL;
    }

    Entity* get_entity(EntityType type, int id, bool clone = false)
    {
        if (type == UserEntity)
            return (id < 0 || id >= M_USERS_MAX_ID || users[id] == NULL) ? NULL : (clone ? new User(*users[id]) : users[id]);
        if (type == LocationEntity)
            return (id < 0 || id >= M_LOCATIONS_MAX_ID || locations[id] == NULL) ? NULL : (clone ? new Location(*locations[id]) : locations[id]);
        if (type == VisitEntity)
            return (id < 0 || id >= M_VISITS_MAX_ID || visits[id] == NULL) ? NULL : (clone ? new Visit(*visits[id]) : visits[id]);
        return NULL;
    }

    void add_entity(Entity *entity)
    {
        if (auto *user = dynamic_cast<User*>(entity))
            add_user(*user);
        else if (auto *location = dynamic_cast<Location*>(entity))
            add_location(*location);
        else if (auto *visit = dynamic_cast<Visit*>(entity))
            add_visit(*visit);
    }

    void update_entity(Entity *entity)
    {
        if (auto *user = dynamic_cast<User*>(entity))
            update_user(*user);
        else if (auto *location = dynamic_cast<Location*>(entity))
            update_location(*location);
        else if (auto *visit = dynamic_cast<Visit*>(entity))
            update_visit(*visit);
    }

    void add_user(const User &_user)
    {
        auto user = new User(_user);
        this->users[user->id] = user;
    }

    void update_user(const User &_user)
    {
        auto user = new User(_user);
        this->users[user->id] = user; // TODO: memory leak
    }

    void add_visit(const Visit &_visit)
    {
        auto visit = new Visit(_visit);
        this->visits[visit->id] = visit;
        this->user_visits[visit->user].push_back(visit);
        this->location_visits[visit->location].push_back(visit);
    }

    void update_visit(const Visit &_visit)
    {
        auto visit = new Visit(_visit);
        auto old_visit = this->visits[visit->id];

        this->visits[visit->id] = visit; // TODO: memory leak

#ifdef DEBUG
        if (visit->id != old_visit->id)
            throw invalid_argument("visait_id mismatch " + to_string(visit->id) + " != " + to_string(old_visit->id));
#endif

        auto &old_user_visits_arr = this->user_visits[old_visit->user];
        for (int i = 0; i < (int) old_user_visits_arr.size(); i++)
        {
            auto &user_visit = old_user_visits_arr[i];
            if (user_visit->id == old_visit->id)
            {
                old_user_visits_arr.erase(old_user_visits_arr.begin() + i);
                break;
            }
        }
        this->user_visits[visit->user].push_back(visit);

        auto &old_location_visits_arr = this->location_visits[old_visit->location];
        for (int i = 0; i < (int) old_location_visits_arr.size(); i++)
        {
            auto &location_visit = old_location_visits_arr[i];
            if (location_visit->id == old_visit->id)
            {
                old_location_visits_arr.erase(old_location_visits_arr.begin() + i);
                break;
            }
        }

        this->location_visits[visit->location].push_back(visit);
    }

    void add_location(const Location &_location)
    {
        auto location = new Location(_location);
        this->locations[location->id] = location;
    }

    void update_location(const Location &_location)
    {
        auto location = new Location(_location);
        this->locations[location->id] = location; // TODO: memory leak
    }

    void fill_from_file(string path, string tmp_dir = "/tmp/data-unpacked")
    {
        fstream in(path + "/options.txt", fstream::in);
        if (in)
        {
            in >> cur_timestamp;
            M_LOG("Read timestamp from options.txt: " << cur_timestamp);
        }
        else
        {
            cur_timestamp = time(NULL);
            M_LOG("optionx.txt not found, use time(NULL): " << cur_timestamp);
        }

        cur_time = localtime(&cur_timestamp);

        path += "/data.zip";

        M_LOG("Removing tmp dir...");
        Utility::system("rm -rf \"" + tmp_dir + "\"");
        M_LOG("Creating tmp dir...");
        Utility::system("mkdir \"" + tmp_dir + "\"");
        M_LOG("Unzipping...");
        Utility::system("unzip -o \"" + path + "\" -d \"" + tmp_dir + "\" > /dev/null");

        DIR *dir;
        dirent *ent;
        if ((dir = opendir(tmp_dir.c_str())) == NULL)
            throw runtime_error("cannot opendir " + tmp_dir);

        M_LOG("Scanning directory...");

        int entities_count[] = {0, 0, 0};

        while ((ent = readdir(dir)) != NULL)
        {
            for (auto &entity_name : vector<string> {"users", "locations", "visits"})
            {
                if (strncmp(entity_name.c_str(), ent->d_name, entity_name.size()) == 0)
                {
                    string fpath = tmp_dir + "/" + ent->d_name;
                    fstream in(fpath, fstream::in);
                    if (!in)
                        throw runtime_error("cannot open file (fstream) " + fpath);
                    M_DEBUG_LOG("Processing " + fpath);

                    string str;
                    getline(in, str, '\0');
                    in.close();

                    while(!str.empty() && str.back() != ']')
                        str.pop_back();
                    if (!str.empty())
                        str.pop_back();
                    while(!str.empty() && isspace(str.back()))
                        str.pop_back();

                    int start_pos = 0;
                    while (start_pos < str.size() && str[start_pos] != '[')
                        start_pos++;
                    start_pos++;

                    while (start_pos < str.size())
                    {
                        json::Object json;
                        if (!json.parse_simple_object(str.c_str(), (int) str.size(), start_pos, start_pos))
                        {
                            M_ERROR("JSON parse error");
                            M_LOG("..." + str.substr(start_pos, 250));
                            exit(1);
                        }
                        start_pos++;
                        while(start_pos < str.size() && isspace(str[start_pos]))
                            start_pos++;
                        while(start_pos < str.size() && str[start_pos] == ',')
                            start_pos++;
                        while(start_pos < str.size() && isspace(str[start_pos]))
                            start_pos++;

                        auto entity_type = entity_name == "users" ? UserEntity : entity_name == "locations" ? LocationEntity : VisitEntity;
                        auto entity = EntityFactory::create(entity_type);
                        if (!entity->parse(json, true))
                            throw runtime_error("invalid data in zip (" + entity_name + ")");
                        add_entity(entity.get());

                        entities_count[entity_type]++;
                    }
                }
            }
        }
        closedir(dir);

        M_LOG("Removing tmp dir...");
        Utility::system("rm -rf \"" + tmp_dir + "\"");

        M_LOG("Total number of users: " << entities_count[UserEntity]);
        M_LOG("Total number of locations: " << entities_count[LocationEntity]);
        M_LOG("Total number of visits: " << entities_count[VisitEntity]);

        M_LOG("Visits per user: " << 1.0 * entities_count[VisitEntity] / entities_count[UserEntity]);
        M_LOG("Visits per location: " << 1.0 * entities_count[VisitEntity] / entities_count[LocationEntity]);

        int u_max = 0;
        for (int i = 0; i < M_USERS_MAX_ID; i++)
            if (users[i] != NULL)
                u_max = max(u_max, (int) user_visits[i].size());

        int l_max = 0;
        for (int i = 0; i < M_LOCATIONS_MAX_ID; i++)
            if (locations[i] != NULL)
                l_max = max(l_max, (int) location_visits[i].size());

        M_LOG("Max user visits: " << u_max);
        M_LOG("Max location visits: " << l_max);
    }

    time_t cur_timestamp;
    tm* cur_time;

    int query_id = 1;
};

extern State state;

#endif //HIGHLOAD_DB_H

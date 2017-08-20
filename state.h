#ifndef HIGHLOAD_DB_H
#define HIGHLOAD_DB_H

#define HASH_TABLE_USERS_MAX_SIZE (1 << 20)
#define HASH_TABLE_LOCATIONS_MAX_SIZE (1 << 20)
#define HASH_TABLE_VISITS_MAX_SIZE (1 << 22)

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
#include "fast_hash_map.h"

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
    FastIntHashMap<User*, HASH_TABLE_USERS_MAX_SIZE> users;
    FastIntHashMap<Location*, HASH_TABLE_LOCATIONS_MAX_SIZE> locations;
    FastIntHashMap<Visit*, HASH_TABLE_VISITS_MAX_SIZE> visits;
    FastIntHashMap<vector<Visit*>, HASH_TABLE_USERS_MAX_SIZE> user_visits;
    FastIntHashMap<vector<Visit*>, HASH_TABLE_LOCATIONS_MAX_SIZE> location_visits;

    bool has_entity(EntityType type, int id)
    {
        return type == UserEntity && users.contains(id)
            || type == LocationEntity && locations.contains(id)
            || type == VisitEntity && visits.contains(id);
    }

    Entity* get_entity(EntityType type, int id, bool clone = false)
    {
        if (type == UserEntity)
            return !users.contains(id) ? NULL : (clone ? new User(*users[id]) : users[id]);
        if (type == LocationEntity)
            return !locations.contains(id) ? NULL : (clone ? new Location(*locations[id]) : locations[id]);
        if (type == VisitEntity)
            return !visits.contains(id) ? NULL : (clone ? new Visit(*visits[id]) : visits[id]);
        return NULL;
    }

    bool add_entity(Entity *entity)
    {
        if (auto *user = dynamic_cast<User*>(entity))
            add_user(*user);
        else if (auto *location = dynamic_cast<Location*>(entity))
            add_location(*location);
        else if (auto *visit = dynamic_cast<Visit*>(entity))
            add_visit(*visit);
        else
            throw invalid_argument("invalid entity type");
    }

    bool update_entity(Entity *entity)
    {
        if (auto *user = dynamic_cast<User*>(entity))
            update_user(*user);
        else if (auto *location = dynamic_cast<Location*>(entity))
            update_location(*location);
        else if (auto *visit = dynamic_cast<Visit*>(entity))
            update_visit(*visit);
        else
            throw invalid_argument("invalid entity type");
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

        if (visit->id != old_visit->id)
            throw invalid_argument("visait_id mismatch " + to_string(visit->id) + " != " + to_string(old_visit->id));

        auto &old_arr = this->user_visits[old_visit->user];
        for (int i = 0; i < (int) old_arr.size(); i++)
        {
            auto &user_visit = old_arr[i];
            if (user_visit->id == old_visit->id)
            {
                old_arr.erase(old_arr.begin() + i);
                break;
            }
        }
        this->user_visits[visit->user].push_back(visit);

        auto &old_arr2 = this->location_visits[old_visit->location];
        for (int i = 0; i < (int) old_arr2.size(); i++)
        {
            auto &location_visit = old_arr2[i];
            if (location_visit->id == old_visit->id)
            {
                old_arr2.erase(old_arr2.begin() + i);
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
        logger::log("Removing tmp dir...");
        system(("rm -rf \"" + tmp_dir + "\"").c_str());
        logger::log("Creating tmp dir...");
        system(("mkdir \"" + tmp_dir + "\"").c_str());
        logger::log("Unzipping...");
        system(("unzip -o \"" + path + "\" -d \"" + tmp_dir + "\"").c_str());

        DIR *dir;
        dirent *ent;
        if ((dir = opendir(tmp_dir.c_str())) == NULL)
            throw runtime_error("cannot opendir " + tmp_dir);

        logger::log("Scanning directory...");

        int entities_count[] = {0, 0, 0};

        while ((ent = readdir(dir)) != NULL)
        {
            for (auto &entity_name : vector<string> {"users", "locations", "visits"})
            {
                if (strncmp(entity_name.c_str(), ent->d_name, entity_name.size()) == 0)
                {
                    string fpath = tmp_dir + "/" + ent->d_name;
                    fstream in(fpath.c_str(), fstream::in);
                    if (!in)
                        throw runtime_error("cannot ioen file (fstream) " + fpath);
                    logger::log("Processing " + fpath);

                    string str;
                    getline(in, str, '\0');
                    logger::log("Data read successful");
                    in.close();

                    while(!str.empty() && str.back() != ']')
                        str.pop_back();
                    if (!str.empty())
                        str.pop_back();
                    while(!str.empty() && isspace(str.back()))
                        str.pop_back();

                    auto tmp = str.c_str();

                    int start_pos = 0;
                    while (start_pos < str.size() && str[start_pos] != '[')
                        start_pos++;
                    start_pos++;

                    while (start_pos < str.size())
                    {
                        json::Object json;
                        if (!json.parse_simple_object(str.c_str(), (int) str.size(), start_pos, start_pos))
                        {
                            logger::error("JSON parse error");
                            logger::log("..." + str.substr(start_pos, 250));
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

        logger::log((string)"Total number of users: " + to_string(entities_count[UserEntity]));
        logger::log((string)"Total number of locations: " + to_string(entities_count[LocationEntity]));
        logger::log((string)"Total number of visits: " + to_string(entities_count[VisitEntity]));
    }
};

extern State state;

#endif //HIGHLOAD_DB_H

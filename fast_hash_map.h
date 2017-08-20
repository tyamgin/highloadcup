#ifndef HIGHLOAD_FAST_HASH_MAP_H
#define HIGHLOAD_FAST_HASH_MAP_H

#include <cstring>
#include <climits>

template<typename T, unsigned max_size>
class FastIntHashMap
{
    int _key[max_size];
    T _value[max_size];
    bool _exists[max_size];

    static unsigned _hash(int key)
    {
        return ((long long)key - INT_MIN) % max_size;
    }

public:
    FastIntHashMap()
    {
        memset(_key, 0, sizeof(_key));
        memset(_exists, 0, sizeof(_exists));
    }

    bool contains(int key) const
    {
        auto hash = _hash(key);
        for (auto h = hash; ; h = (h + 1) % max_size)
        {
            if (!_exists[h])
                return false;

            if (_key[h] == key)
                return true;
        }
    }

    T &operator [](int key)
    {
        auto hash = _hash(key);
        for (auto h = hash; ; h = (h + 1) % max_size)
        {
            if (!_exists[h])
            {
                _exists[h] = true;
                _key[h] = key;
                return _value[h];
            }

            if (_key[h] == key)
            {
                return _value[h];
            }
        }
    }
};

#endif //HIGHLOAD_FAST_HASH_MAP_H

#ifndef HIGHLOAD_UTILITY_H
#define HIGHLOAD_UTILITY_H

#include <string>
#include <climits>
#include <vector>
#include <sstream>
#include <iomanip>

using namespace std;

#define INT_MAX_LENGTH 11

#define M_STRLEN(str) (sizeof(str) - 1)
#define M_MEMCPY(dest, src) memcpy((dest), (src), sizeof((src)))

typedef unsigned long long HashType;

#define M_STRING_HASH_BASE 263

class Utility
{
public:
    static bool tryParseInt(const char* str, int &result)
    {
        if (str[0] == 0)
            return false;

        if (str[0] != '-' && isdigit(str[0]) == 0)
            return false;

        for(int i = 1; str[i] != 0; i++)
        {
            if (isdigit(str[i]) == 0)
                return false;
            if (i > INT_MAX_LENGTH)
                return false;
        }

        auto ll_val = atoll(str);
        if (ll_val < INT_MIN || ll_val > INT_MAX)
            return false;

        result = (int) ll_val;
        return true;
    }


    static bool is_int1(const char* str)
    {
        if (str[0] == 0)
            return false;

        if (str[0] != '-' && isdigit(str[0]) == 0)
            return false;

        for(int i = 1; str[i] != 0; i++)
        {
            if (isdigit(str[i]) == 0)
                return false;
            if (i > 12)
                return false;
        }

        auto ll_val = atoll(str);
        return ll_val >= INT_MIN && ll_val <= INT_MAX;
    }

    static HashType stringUrlDecodedHash(const char* src)
    {
        char a, b;
        HashType ret = 0, p = 1;

        while (*src)
        {
            if ((*src == '%') &&
                ((a = src[1]) && (b = src[2])) &&
                (isxdigit(a) && isxdigit(b)))
            {
                if (a >= 'a')
                    a -= 'a' - 'A';
                if (a >= 'A')
                    a -= ('A' - 10);
                else
                    a -= '0';
                if (b >= 'a')
                    b -= 'a' - 'A';
                if (b >= 'A')
                    b -= ('A' - 10);
                else
                    b -= '0';

                ret = ret * p + HashType(16 * a + b);

                src += 3;
            }
            else if (*src == '+')
            {
                ret = ret * p + (HashType) static_cast<unsigned char>(' ');
                src++;
            }
            else
            {
                ret = ret * p + (HashType) static_cast<unsigned char>(*src++);
            }
            p *= M_STRING_HASH_BASE;
        }
        return ret;
    }

    static void system(const string &cmd)
    {
        int ret = ::system(cmd.c_str());
    }
};


#endif //HIGHLOAD_UTILITY_H

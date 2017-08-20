#ifndef HIGHLOAD_UTILITY_H
#define HIGHLOAD_UTILITY_H

#include <string>
#include <climits>
#include <vector>
#include <sstream>
#include <iomanip>

using namespace std;

class Utility
{
public:
    static bool is_int(const char* str)
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

    static bool startsWith(const string &subj, const string &substr)
    {
        if (substr.size() > subj.size())
            return false;

        for(int i = 0; i < (int) substr.size(); i++)
            if (substr[i] != subj[i])
                return false;
        return true;
    }

    static bool endsWith(const string &subj, const string &substr)
    {
        if (substr.size() > subj.size())
            return false;

        for(int i = 0; i < (int) substr.size(); i++)
            if (substr[i] != subj[(int)subj.size() - (int)substr.size() + i])
                return false;
        return true;
    }

    template<typename Out>
    static void split(const string &str, char delim, Out result)
    {
        stringstream ss;
        ss.str(str);
        string item;
        while (getline(ss, item, delim))
            *(result++) = item;
    }

    static vector<string> split(const string &s, char delim)
    {
        std::vector<string> elems;
        split(s, delim, back_inserter(elems));
        return elems;
    }

    static string urlDecode(const string &_src)
    {
        char a, b;
        auto src = _src.c_str();
        string res;
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
                res += 16 * a + b;
                src += 3;
            }
            else if (*src == '+')
            {
                res += ' ';
                src++;
            }
            else
            {
                res += *src++;
            }
        }
        return res;
    }
};


#endif //HIGHLOAD_UTILITY_H

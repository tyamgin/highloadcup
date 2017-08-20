#ifndef HIGHLOAD_LOGGER_H
#define HIGHLOAD_LOGGER_H

#include <iostream>
using namespace std;

namespace logger
{
    template<typename T>
    void log(const T &s)
    {
        cout << s << endl;
    }

    template<typename T>
    void error(const T &s)
    {
        cout << "!!! " << s << endl;
    }
}

#endif //HIGHLOAD_LOGGER_H

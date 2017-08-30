#ifndef HIGHLOAD_JSON_H
#define HIGHLOAD_JSON_H

#include <iostream>
#include <string>
#include <vector>
#include <exception>
#include <stdexcept>
#include <map>
#include <cstring>
#include <cassert>

using namespace std;

namespace json
{
    class Object;
    class Value;

    enum ValueType
    {
        StringVal,
        IntVal
    };

    class Value
    {
        ValueType _type;
        char* _val;
        int _int_val;

        void _copy_value(const char* src, int begin, int end)
        {
            int length = end - begin;
            _val = new char[length + 1];
            memcpy(_val, src + begin, length);
            _val[length] = 0;
        }
    public:

        ~Value()
        {
            //delete _val; // skip memory leak
        }

        Value(const Value &value)
        {
            _type = value._type;
            if (value._val != NULL)
                //_copy_value(value._val, 0, strlen(value._val));
                _val = value._val;
            else
                _val = NULL;
            _int_val = value._int_val;
        }

        Value(Value &&value)
        {
            _type = value._type;
            _val = value._val;
            _int_val = value._int_val;
            value._val = NULL;
        }

        Value()
        {
            _val = NULL;
            _int_val = 0;
            _type = IntVal;
        }

        ValueType GetType() const
        {
            return _type;
        }

        explicit operator char*() const
        {
            assert(_type == StringVal);
            return _val;
        }

        explicit operator int() const
        {
            assert(_type == IntVal);
            return _int_val;
        }

        friend class Object;
    };

    class Object
    {
#define _expect(str, pos, expected) if (str[pos] != (expected)) goto fail;

        bool _parse_simple_object(const char *str, int n, int &end_pos)
        {
            int pos = 0;
            char *key = NULL;

            while (isspace(str[pos])) pos++;

            _expect(str, pos++, '{');

            while (isspace(str[pos])) pos++;

            while (str[pos] != '}')
            {
                key = NULL;
                Value value;

                _expect(str, pos++, '"');

                int key_start_pos = pos;
                while (pos < n && str[pos] != '"')
                    pos++;
                key = new char[pos - key_start_pos + 1];
                memcpy(key, str + key_start_pos, pos - key_start_pos);
                key[pos - key_start_pos] = 0;

                _expect(str, pos++, '"');

                while (isspace(str[pos])) pos++;
                _expect(str, pos++, ':');
                while (isspace(str[pos])) pos++;

                if (str[pos] == '-' || isdigit(str[pos]))
                {
                    // IntVal
                    bool is_negative = false;
                    if (str[pos] == '-')
                        is_negative = true, pos++;

                    // TODO: check any digit exists
                    while (isdigit(str[pos]))
                        value._int_val = value._int_val * 10 + str[pos++] - '0';

                    if (is_negative)
                        value._int_val *= -1;
                    value._type = IntVal;
                }
                else
                {
                    // StringVal
                    _expect(str, pos++, '"');

                    int value_start_pos = pos;
                    while (pos < n && str[pos] != '"')
                        pos++;

                    value._copy_value(str, value_start_pos, pos);

                    _expect(str, pos++, '"');
                    value._type = StringVal;
                }

                while (isspace(str[pos])) pos++;

                if (str[pos] == ',')
                    pos++;

                while (isspace(str[pos])) pos++;

                properties.emplace_back(key, value);
            }
            end_pos = pos;

            return true;
fail:
            delete key;
            return false;
        }

    public:
        vector<pair<char*, Value> > properties;

        bool parse_simple_object(const char* str, int length)
        {
            int end_pos;
            return _parse_simple_object(str, length, end_pos);
        }

        bool parse_simple_object(const char* str, int length, int start_pos, int &end_pos)
        {
            auto ret = _parse_simple_object(str + start_pos, length - start_pos, end_pos);
            end_pos += start_pos;
            return ret;
        }

        ~Object()
        {
            for (auto &pair : properties)
                delete pair.first;
        }
    };
};

#endif //HIGHLOAD_JSON_H

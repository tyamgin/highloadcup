#ifndef HIGHLOAD_JSON_H
#define HIGHLOAD_JSON_H

#include <iostream>
#include <string>
#include <vector>
#include <exception>
#include <stdexcept>
#include <string.h>
#include <map>
#include <assert.h>

using namespace std;

namespace json
{
    class json_parse_error : public runtime_error
    {
        string _msg;
        int _pos;

    public:
        json_parse_error(const string &msg, int pos) : runtime_error("JSON parse error"), _msg(msg), _pos(pos)
        {
        }

        int get_pos() const
        {
            return _pos;
        }

        const char* what() const throw() override
        {
            string txt = (string)runtime_error::what() + ": " + _msg + " at position " + to_string(_pos);
            auto _what_cache = new char[txt.size() + 1];
            strcpy(_what_cache, txt.c_str());

            return (const char*)_what_cache; // TODO: memory leak
        }
    };


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
            if (_val != NULL)
                delete _val;
        }

        Value(const Value &value)
        {
            _type = value._type;
            if (value._val != NULL)
                _copy_value(value._val, 0, strlen(value._val));
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

        explicit operator const char*() const
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
        void _expect(const char *str, int pos, char expected)
        {
            if (str[pos] != expected)
            {
                string en;
                if (str[pos] != 0)
                    en += str[pos];
                else
                    en += "\\0";
                throw json_parse_error((string)"expected '" + expected + "', but found '" + en + "'", pos);
            }
        }

        void _parse_simple_object(const char *str, int n, int &end_pos)
        {
            int pos = 0;

            while (isspace(str[pos])) pos++;

            _expect(str, pos++, '{');

            while (isspace(str[pos])) pos++;

            while (str[pos] != '}')
            {
                string key;
                Value value;

                _expect(str, pos++, '"');

                while(pos < n && str[pos] != '"')
                {
                    key += str[pos];
                    pos++;
                }
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
                    while(pos < n && str[pos] != '"')
                    {
                        value._val += str[pos];
                        pos++;
                    }
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
        }

    public:
        vector<pair<string, Value> > properties;

        void parse_simple_object(const char* str, int length)
        {
            int end_pos;
            _parse_simple_object(str, length, end_pos);
        }

        void parse_simple_object(const char* str, int length, int start_pos, int &end_pos)
        {
            _parse_simple_object(str + start_pos, length - start_pos, end_pos);
            end_pos += start_pos;
        }
    };
};

#endif //HIGHLOAD_JSON_H

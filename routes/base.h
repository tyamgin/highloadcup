#ifndef HIGHLOAD_BASE_H
#define HIGHLOAD_BASE_H

#include <string>

using namespace std;

class RouteProcessor
{
public:
    int status_code = 200;
    string body;

    void handle_404()
    {
        status_code = 404;
        body = "{}";
    }

    void handle_400()
    {
        status_code = 400;
        body = "{}";
    }

    string get_http_result()
    {
        string result;
        switch (status_code)
        {
            case 404:
                result += "HTTP/1.1 404 Not Found\nContent-Type: application/json; charset=utf-8\n";
                break;
            case 400:
                result += "HTTP/1.1 400 Bad Request\nContent-Type: application/json; charset=utf-8\n";
                break;
            default:
                result += "HTTP/1.1 200 OK\nContent-Type: application/json; charset=utf-8\n";
        }


        result += "Content-Length: " + to_string(body.size()) + "\n";
        result += "\n";
        result += body;

        return result;
    }
};


#endif //HIGHLOAD_BASE_H

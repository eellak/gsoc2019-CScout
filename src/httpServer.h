#ifndef HTTPSERV_H_
#define HTTPSERV_H_
#include <cpprest/http_listener.h>
#include <cpprest/json.h>
#include <string>
#include <functional>

using namespace std;
using namespace web;
using namespace web::http;
using namespace web::http::experimental::listener;
 

typedef struct{
    utility::string_t value;
<<<<<<< HEAD
    function <json::value(json::value*)> handleFunction;
    json::value* attributes;
=======
    function <json::value(void*)> handleFunction;
    void* attributes;
>>>>>>> d27152392df5acdff3428036dc3fac2c7e65438f
} Handler;


class HttpServer{

public:
    HttpServer(){}
    HttpServer(utility::string_t url);
    void handle_get(http_request request);
    void handle_post(http_request request);
    void handle_delete(http_request request);
    void handle_put(http_request request);
<<<<<<< HEAD
    void addHandler(utility::string_t value,function <json::value(json::value*)> handleFunction,json::value* attributes);
=======
    void addHandler(utility::string_t value,function <json::value(void*)> handleFunction,void* attributes);
>>>>>>> d27152392df5acdff3428036dc3fac2c7e65438f
    void serve();
    http_listener listener;

};
#endif
#ifndef HTTPSERV_H_
#define HTTPSERV_H_
#include <cpprest/http_listener.h>
#include <cpprest/json.h>
#include <string>
#include <functional>
#include <sys/wait.h>

using namespace std;
using namespace web;
using namespace web::http;
using namespace web::http::experimental::listener;
 
class HttpServer;

// Struct to hold functions to respond  
typedef struct{
    utility::string_t value;
    function <json::value(void*)> handleFunction;
    void* attributes;
} Handler;


class HttpServer{
private : 
    json::value params;
    ofstream *log_file;
    static bool must_exit;
public:
    HttpServer(){}
    //Constructor for HttpServer on a url
    HttpServer(utility::string_t url, ofstream * log);
    //HTTP GET handler
    void handle_get(http_request request);
    //HTTP POST handler
    void handle_post(http_request request);
    void handle_delete(http_request request);
    void handle_put(http_request request);
    //adds a function to a map based on the path of the request
    void addHandler(utility::string_t value,function <json::value(void*)> handleFunction,void* attributes);
    //adds a function to a map based on the path of the request for Http Put
    void addPutHandler(utility::string_t value,function <json::value(void *)> handleFunction,void* attributes);
    //starts the server to listen on the url
    void serve();
    //get request parameter from uri as int
    int getIntParam(string name);
    //get request parameter from uri as string
    string getStrParam(string name);
    //get request parameter from uri as a pointer
    void * getAddrParam(string name);
    //get request paramete from uri as bool
    bool getBoolParam(string name);
    //log server to a file
    void log(string msg);
    // listener sturct 
    http_listener listener;

};
#endif

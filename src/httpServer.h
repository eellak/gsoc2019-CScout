#include <cpprest/http_listener.h>
#include <cpprest/json.h>
#include <string>


using namespace std;
using namespace web;
using namespace web::http;
using namespace web::http::experimental::listener;

class httpServer{
public:
    httpServer(){}
    httpServer(utility::string_t url);

private:
    void handle_get(http_request request);
    void handle_post(http_request request);
    void handle_delete(http_request request);
    void handle_put(http_request request);

    http_listener listener;
};
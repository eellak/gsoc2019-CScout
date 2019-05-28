#include "httpServer.h"


HttpServer::HttpServer(utility::string_t url) : listener(url){
    listener.support(methods::GET, std::bind(&HttpServer::handle_get,this, std::placeholders::_1));
    listener.support(methods::POST, std::bind(&HttpServer::handle_post,this, std::placeholders::_1));
    listener.support(methods::DEL, std::bind(&HttpServer::handle_delete,this, std::placeholders::_1));
    listener.support(methods::PUT, std::bind(&HttpServer::handle_put,this, std::placeholders::_1));
}

void HttpServer::handle_get(http_request request){
 /* to add handler */
}


void HttpServer::handle_post(http_request request){
 /* to add handler */
}


void HttpServer::handle_put(http_request request){
 /* to add handler */
}


void HttpServer::handle_delete(http_request request){
 /* to add handler */
}
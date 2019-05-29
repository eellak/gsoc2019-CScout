#include "httpServer.h"
map<utility::string_t, Handler> handler_dictionary;

HttpServer::HttpServer(utility::string_t url) : listener(url){
    
    listener.support(methods::GET, std::bind(&HttpServer::handle_get,this, std::placeholders::_1));
    listener.support(methods::POST, std::bind(&HttpServer::handle_post,this, std::placeholders::_1));
    listener.support(methods::DEL, std::bind(&HttpServer::handle_delete,this, std::placeholders::_1));
    listener.support(methods::PUT, std::bind(&HttpServer::handle_put,this, std::placeholders::_1)); 
    cout << "HttpServer: constructor called listen at "<< url << endl;
}

void HttpServer::addHandler(utility::string_t value,function <json::value(void*)> handleFunction,void* attributes){
    Handler funcHandler;
    funcHandler.value = value;
    funcHandler.handleFunction = handleFunction;
    funcHandler.attributes = attributes;
    handler_dictionary[value] = funcHandler;
    //handler_dictionary.insert(pair<utility::string_t,Handler>(value,funcHandler));   
    cout << "HttpServer: addHandler " << value<< " called \n";
}

void HttpServer::serve(){
    http_listener * list = &(this->listener);
    cout <<"HttpServer: serve begin. Mapped functions\n";
    for(auto it = handler_dictionary.begin(); it != handler_dictionary.end(); it++){
        cout << it->first <<" - " << it->second.value << endl;
    }
    try{
        this->listener.open()
            .then([&list](){cout << "\n Http Rest Server starts listening \n";})
            .wait();
        
        while(true);
    }
    catch (exception const & e){
        cout << e.what() << endl;
    }
}

void HttpServer::handle_get(http_request request){
 /* to add handler */

    utility::string_t path = request.relative_uri().path();
    cout << "HttpServer: Handle get of "<<path << endl;
    cout <<"HttpServer: Get begin. Mapped functions\n";
  
    auto it = handler_dictionary.find(path.substr(1));

    if (it == handler_dictionary.end())
        cout << "Empty iterator" << endl;
    else
        cout<<"HttpServer:handle_get: handler:"<<it->first << endl;
    json::value response = it->second.handleFunction(it->second.attributes);

    request.reply(status_codes::OK, response);
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
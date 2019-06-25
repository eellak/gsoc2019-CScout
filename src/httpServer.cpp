#include "headers.h"

bool HttpServer::must_exit = false;

// URI Path to func dictionary 
map<utility::string_t, Handler> handler_dictionary;
map<utility::string_t, Handler> put_handler_dictionary;

//Server constructor binds handlers to methods
HttpServer::HttpServer(utility::string_t url, ofstream * log) : listener(url),log_file(log){
    listener.support(methods::GET, std::bind(&HttpServer::handle_get,this, std::placeholders::_1));
    listener.support(methods::POST, std::bind(&HttpServer::handle_post,this, std::placeholders::_1));
    listener.support(methods::DEL, std::bind(&HttpServer::handle_delete,this, std::placeholders::_1));
    listener.support(methods::PUT, std::bind(&HttpServer::handle_put,this, std::placeholders::_1)); 
    cerr << "HttpServer: constructor called listen at "<< url << endl;
}

//Adds functions to URI path Handlers
void HttpServer::addHandler(utility::string_t value,function <json::value(void *)> handleFunction,void* attributes){
    Handler funcHandler;
    funcHandler.value = value;
    funcHandler.handleFunction = handleFunction;
    funcHandler.attributes = attributes;
    handler_dictionary[value] = funcHandler; 
    cerr << "HttpServer: addHandler " << value<< " called \n";
}

void HttpServer::addPutHandler(utility::string_t value,function <json::value(void *)> handleFunction,void* attributes){
    Handler funcHandler;
    funcHandler.value = value;
    funcHandler.handleFunction = handleFunction;
    funcHandler.attributes = attributes;
    put_handler_dictionary[value] = funcHandler; 
    cerr << "HttpServer: addHandler " << value<< " called \n";
}

// Server start listening
void HttpServer::serve(){
    http_listener * list = &(this->listener);


    try{
        this->listener.open()
            .then([&list](){cerr << "\n Http Rest Server starts listening \n";})
            .wait(); 

        while(!must_exit)
            wait(NULL);
        this->listener.close().wait();
            
    }
    catch (exception const & e){
        cerr << e.what() << endl;
    }
}

// HTTP GET handler
void HttpServer::handle_get(http_request request){
    
    // cout<<"URI:"<<request.absolute_uri().to_string()<<endl;;
    utility::string_t path = request.relative_uri().path();
    // cout << "HttpServer: Handle get of "<<path << endl;
    json::value response;

    cerr << "HttpServer: Handle get of "<<path << endl;
    // cout <<"HttpServer: Get begin. Mapped functions\n";
  
    // match path with dictionary
    auto it = handler_dictionary.find(path.substr(1));
    if (it == handler_dictionary.end()){
       
        response["error"] = json::value::string("Url Not Found");
        request.reply(status_codes::NotFound,response);
        if(this->log_file != NULL){
            cerr<<"write to log"<<endl;
            *(this->log_file)<<response.serialize();
        }
    }
    else{
        cerr<<"HttpServer:handle_get: handler:"<<it->first << endl;
        cerr<<"URI:"<< request.request_uri().query()<<endl;
        //Uri params to json value
        std::map<utility::string_t, utility::string_t> attributes = web::uri::split_query(request.request_uri().query());
        
        // cout << "Attributes:" << endl;

        for(std::map<utility::string_t, utility::string_t>::iterator it = attributes.begin(); it != attributes.end(); it++){
            // cout<<"start to go through:"<<server.params.serialize() << endl;
            server.params[it->first] = json::value::string(it->second);
            // cout<<it->first << "-"<<it->second<<endl;
        }
        // cout << "JSON:"<< server.params.serialize().c_str() << endl;
        response = it->second.handleFunction(&(server.params));
        request.reply(status_codes::OK, response);
        // cout << "Get Response:"<< response.serialize().c_str() << endl;
        if(this->log_file != NULL){
            cerr<<"write to log"<<endl;
            *(this->log_file)<<response.serialize();
        }
    }
   server.params=json::value(); 
}


void HttpServer::handle_put(http_request request) {
 /* to add handler */
    utility::string_t path = request.relative_uri().path();
    auto it = put_handler_dictionary.find(path.substr(1));
    json::value response;
    
    if (it == put_handler_dictionary.end()){
        response["error"] = json::value::string("Url Not Found");
        request.reply(status_codes::NotFound,response);
        if(this->log_file != NULL){
            cerr<<"write to log"<<endl;
            *(this->log_file)<<response.serialize();
        }
    }
    else{
        cerr<<"HttpServer:handle_put: handler:"<<it->first << endl;
        cerr<<"URI:"<< request.request_uri().query()<<endl;
        std::map<utility::string_t, utility::string_t> attributes = web::uri::split_query(request.request_uri().query());
        
        // cout << "Attributes:" << endl;

        for(std::map<utility::string_t, utility::string_t>::iterator it = attributes.begin(); it != attributes.end(); it++){
            // cout<<"start to go through:"<<server.params.serialize() << endl;
            server.params[it->first] = json::value::string(it->second);
            // cout<<it->first << "-"<<it->second<<endl;
        }
        response = it->second.handleFunction(&(server.params));
        bool exodus = response.has_field("exit");
        request.reply(status_codes::OK, response).then([&exodus](){
            if(exodus){
                must_exit = true;
                //throw exodus;
            }
        });
        // cout << "Get Response:"<< response.serialize().c_str() << endl;
        // cout<<(this->log_file == NULL);
        if(this->log_file != NULL){
            cerr<<"write to log"<<endl;
            *(this->log_file)<<response.serialize();
        }
               
    }

    server.params=json::value();
}

//Read Uri parameter as an address
void * HttpServer::getAddrParam(string name){
     if(!(server.params.has_field(name))){
        // cout<<"zero"<<endl;
        return NULL;
    }
    else
        if(server.params[name].is_null())
            return NULL;
        else{
            void * p;
            sscanf(server.params[name].as_string().c_str(),"%p",  &p);
            return p;
        }

}

//Read Uri parameter as an integer
int HttpServer::getIntParam(string name){
    // cout<<"Get int "<<name<<"-"<< server.params.has_field(name)<<endl;
    if(!(server.params.has_field(name))){
        // cout<<"zero"<<endl;
        return -1;
    }
    else
        if(server.params[name].is_null())
            return -1;
        else{
            // cout<<server.params[name].as_string()<<endl;
            return stoi(server.params[name].as_string());
        }
}

//Check if parameter exists in Uri
bool HttpServer::getBoolParam(string name){
    return server.params.has_field(name);
}

//Read Uri Parameter as a string
string  HttpServer::getStrParam(string name){
    // cout<<"Get str "<<name<<"-"<< server.params.has_field(name)<<endl;
    if(!(server.params.has_field(name))){
        // cout<<"zero"<<endl;
        return {};
    }
    else{
        // cout << "params[name]:"<< server.params[name].serialize() << endl;
    }
        if(server.params[name].is_null())
            return {};
        else{
            // cout <<"name:"<< server.params[name].as_string()<<endl;
            return server.params[name].as_string(); 
        }
}

//Read Uri Parameter as a string
const char * HttpServer::getCharPParam(string name){
    // cout<<"Get str "<<name<<"-"<< server.params.has_field(name)<<endl;
    if(!(server.params.has_field(name))){
        // cout<<"zero"<<endl;
        return NULL;
    }
    else{
        // cout << "params[name]:"<< server.params[name].serialize() << endl;
    }
        if(server.params[name].is_null())
            return NULL;
        else{
            char *c = new char[server.params[name].as_string().length()+1];
            strcpy(c,server.params[name].as_string().c_str());
            // cout <<"name:"<< server.params[name].as_string()<<endl;
            return c; 
        }
}


void HttpServer::handle_post(http_request request){
 /* to add handler */
}

void HttpServer::handle_delete(http_request request){
 /* to add handler */
}

void HttpServer::log(string msg){
//    cout<<"logging"<<endl;
   if(this->log_file != NULL){
        cerr<<"write to log"<<endl;
        *(this->log_file)<<msg<<endl;
    } 

}
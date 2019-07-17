#include "headers.h"

bool HttpServer::must_exit = false;

// URI Path to func dictionary
map<utility::string_t, Handler> handler_dictionary;
map<utility::string_t, Handler> put_handler_dictionary;

//Server constructor binds handlers to methods
HttpServer::HttpServer(utility::string_t url, ofstream *log) : listener(url), log_file(log)
{
    listener.support(methods::GET, std::bind(&HttpServer::handle_get, this, std::placeholders::_1));
    listener.support(methods::PUT, std::bind(&HttpServer::handle_put, this, std::placeholders::_1));
    if(DP())
        cerr << "HttpServer: constructor called listen at " << url << endl;
}

//Adds functions to URI path Handlers
void HttpServer::addHandler(utility::string_t value, function<json::value(void *)> handleFunction, void *attributes)
{
    Handler funcHandler;
    funcHandler.value = value;
    funcHandler.handleFunction = handleFunction;
    funcHandler.attributes = attributes;
    handler_dictionary[value] = funcHandler;
    if(DP())
        cerr << "HttpServer: addHandler " << value << " called \n";
}

void HttpServer::addPutHandler(utility::string_t value, function<json::value(void *)> handleFunction, void *attributes)
{
    Handler funcHandler;
    funcHandler.value = value;
    funcHandler.handleFunction = handleFunction;
    funcHandler.attributes = attributes;
    put_handler_dictionary[value] = funcHandler;
    if(DP())
        cerr << "HttpServer: addPutHandler " << value << " called \n";
}

// Server start listening
void HttpServer::serve()
{
    http_listener *list = &(this->listener);

    try
    {
        this->listener.open()
            .then([&list]() { cerr << "\n Http Rest Server starts listening \n"; })
            .wait();

        while (!must_exit)
            wait(NULL);
        this->listener.close().wait();
    }
    catch (exception const &e)
    {
        cerr << e.what() << endl;
    }
}

// HTTP GET handler
void HttpServer::handle_get(http_request request)
{

    
    utility::string_t path = request.relative_uri().path();
    if(DP())
        cout << "URI:" << request.absolute_uri().to_string() << endl
        << "HttpServer: Handle get of " <<path << endl;
    http_response response;
    json::value body;

    cerr << "HttpServer: Handle get of " << path << endl;
    if(DP())
        cout <<"HttpServer: Get begin. Mapped functions\n";

    // match path with dictionary
    auto it = handler_dictionary.find(path.substr(1));
    if (it == handler_dictionary.end())
    {
        response = http_response(status_codes::NotFound);
        body["error"] = json::value::string("Url Not Found");
        response.set_body(body);
        if(DP())
            cout << "response:" << response.to_string() << endl;
        request.reply(response);
        if (this->log_file != NULL)
        {
            
            cerr << "write to log" << endl;
            *(this->log_file) << body.serialize();
        }
    }
    else
    {
        cerr << "HttpServer:handle_get: handler:" << it->first << endl;
        utility::string_t dec_query = web::uri::decode(request.request_uri().query());
        cerr << "URI:" << dec_query << endl;
        //Uri params to json value
        std::map<utility::string_t, utility::string_t> attributes = web::uri::split_query(dec_query);

        if(DP())
            cout << "Attributes:" << endl;

        for (std::map<utility::string_t, utility::string_t>::iterator it = attributes.begin(); it != attributes.end(); it++)
        {           
            server.params[it->first] = json::value::string(it->second);
        }
        if(DP())
            cout << "JSON:"<< server.params.serialize().c_str() << endl;
        response = http_response(status_codes::OK);
        body = it->second.handleFunction(&(server.params));
        response.headers().add(U("Access-Control-Allow-Origin"), U("*"));
        response.set_body(body);
        if(DP())
            cout << "response:" << response.to_string() << endl;
        request.reply(response);
        if (this->log_file != NULL)
        {
            cerr << "write to log" << endl;
            *(this->log_file) << body.serialize();
        }
    }
    server.params = json::value();
}

// HTTP PUT handler
void HttpServer::handle_put(http_request request)
{
    utility::string_t path = request.relative_uri().path();
    auto it = put_handler_dictionary.find(path.substr(1));
    json::value body;
    http_response response;

    if (it == put_handler_dictionary.end())
    {
        body["error"] = json::value::string("Url Not Found");
        response = http_response(status_codes::NotFound);
        response.set_body(body);
        if(DP())
            cout << "response:" << response.to_string() << endl;
        request.reply(response);
        if (this->log_file != NULL)
        {
            cerr << "write to log" << endl;
            *(this->log_file) << body.serialize();
        }
    }
    else
    {
        cerr << "HttpServer:handle_put: handler:" << it->first << endl;
         utility::string_t dec_query = web::uri::decode(request.request_uri().query());
        cerr << "URI:" << dec_query << endl;
        std::map<utility::string_t, utility::string_t> attributes = web::uri::split_query(dec_query);

        if(DP())
            cout << "Attributes:" << endl;

        for (std::map<utility::string_t, utility::string_t>::iterator it = attributes.begin(); it != attributes.end(); it++)
        {
            server.params[it->first] = json::value::string(it->second);
        }
        if(DP())
            cout << "JSON:"<< server.params.serialize().c_str() << endl;
        response = http_response(status_codes::OK);
        body = it->second.handleFunction(&(server.params));
        response.headers().add(U("Access-Control-Allow-Origin"), U("http://localhost:3000"));
        response.set_body(body);
        if(DP())
            cout << "response:" << response.to_string() << endl;
        request.reply(response);

        bool exodus = body.has_field("exit");
        request.reply(response).then([&exodus]() {
            if (exodus)
            {
                must_exit = true;
                //throw exodus;
            }
        });
 
        if (this->log_file != NULL)
        {
            cerr << "write to log" << endl;
            *(this->log_file) << body.serialize();
        }
    }

    server.params = json::value();
}

 // Returns address parameter from uri
void *HttpServer::getAddrParam(string name)
{
    if (!(server.params.has_field(name)))
    {
        if(DP())
            cout << "no parameter named " << name << endl;
        return NULL;
    }
    else if (server.params[name].is_null()) {
        if(DP())
            cout << "parameter named " << name << " is NULL" << endl;
        return NULL;
    }
    else
    {
        void *p;
        sscanf(server.params[name].as_string().c_str(), "%p", &p);
        if(DP())
            cout << "parameter" << name << " = "<< p << endl;
        return p;
    }
}

// Returns integer request parameter from uri
int HttpServer::getIntParam(string name)
{
    if (!(server.params.has_field(name)))
    {
        if(DP())
            cout << "no parameter named " << name << endl;
        return -1;
    }
    else if (server.params[name].is_null()) {
        if(DP())
            cout << "parameter named " << name <<  "is NULL" << endl;
        return -1;
    }
    else
    {
        if(DP())
            cout << "parameter" << name <<" = " << stoi(server.params[name].as_string()) << endl;
        return stoi(server.params[name].as_string());
    }
}

// Returns true if parameter exists in uri
bool HttpServer::getBoolParam(string name)
{
    return server.params.has_field(name);
}

// Returns string Parameter query from uri
string HttpServer::getStrParam(string name)
{
    if (!(server.params.has_field(name)))
    {
        if(DP())
            cout << "no parameter named " << name << endl;
        return {};
    }
    if (server.params[name].is_null())
    {
        if(DP())
            cout << "parameter named " << name << " is NULL" << endl;
        return {};
    }
    else
    {
        if(DP())
            cout << "parameter " << name << " = " << server.params[name].as_string() << endl;
        return server.params[name].as_string();
    }
}

// Returns const char* Parameter query from uri
const char *HttpServer::getCharPParam(string name)
{
    if (!(server.params.has_field(name)))
    {
        if(DP())
            cout << "no parameter named " << name << endl;
        return NULL;
    }
    if (server.params[name].is_null())
    {
        if(DP())
            cout << "parameter named " << name << " is NULL" << endl;
        return NULL;
    }
    else
    {
        char *c = new char[server.params[name].as_string().length() + 1];
        strcpy(c, server.params[name].as_string().c_str());
        if(DP())
            cout << "parameter " << name << " = " << c << endl;
        return c;
    }
}


// Write msg to log file
void HttpServer::log(string msg)
{
    if (this->log_file != NULL)
    {
        cerr << "write to log" << endl;
        *(this->log_file) << msg << endl;
    }
}
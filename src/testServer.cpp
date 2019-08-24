#include <cpprest/http_client.h>
#include <cpprest/json.h>
#include <csignal>
#include <unistd.h>

using namespace web;
using namespace web::http;
using namespace web::http::client;

#include <iostream>
using namespace std;

pplx::task<http_response> make_task_request(http_client &client, method mtd, const char *path)
{
   return client.request(mtd, path);
}

void sigHandler(int signum)
{
   if(signum == SIGUSR1){
      cout << "Begin Sending requests" << endl;
      usleep(100000);
   }
}


void to_query(json::value jvalue, uri_builder *builder)
{

   json::object query = jvalue.as_object();
   for (auto i = query.cbegin(); i != query.cend(); i++)
   {
      if(i->second.is_string())
         builder->append_query(i->first, i->second.as_string());
      else
      {
         builder->append_query(i->first, i->second.serialize()); 
      }
      
   }
}
bool check_valid(json::value response, const char *path)
{
   if (response.size() == 0 || response.as_object().empty())
   {
      return false;
   }
   if (response.has_field("error"))
   {
      return false;
   }
   std::fstream fs;
   // fs.open (strcat("./test/preOut/",path), std::fstream::in);
   // fs.close();
   return true;
}

bool make_request(
    http_client &client,
    method mtd, const char *path,
    json::value &jvalue, json::value req, int no)
{

   bool valid = false;
   string *s;

   //check if query needs info from another one
   if (req.has_field("dependant"))
   {
      make_task_request(client, methods::GET, req["dependant"].as_string().c_str())
          .then([](http_response response) {
             if (response.status_code() == status_codes::OK)
             {
                return response.extract_json();
             }
             return pplx::task_from_result(json::value());
          })
          .then([&jvalue, &req, &path, &s, &client, &mtd, &no](pplx::task<json::value> previousTask) {
             try
             {
                json::value returned = previousTask.get();

                // string *s;

                if (returned.has_string_field("addr"))
                {
                   s = new string(returned["addr"].as_string());
                }
                for (auto i = req["dependantQuery"].as_array().cbegin();
                     i != req["dependantQuery"].as_array().cend(); i++)
                {
                  if(i->is_string()){ 
                     if (returned.has_array_field(i->as_string()))
                        for (int j = 0; j < returned[i->as_string()].size(); j++)
                        {

                           if (returned[i->as_string()][j].is_string())
                              jvalue[i->as_string()] = returned[i->as_string()][j];
                           else
                              jvalue[i->as_string()] = json::value::string(returned[i->as_string()][j].serialize());
                           if (!make_request(client, mtd, path, jvalue, json::value(), no++))
                              return false;
                        }
                     else
                        jvalue[i->as_string()] = returned[i->as_string()];
                  }
                  else {
                     json::object dep = i->as_object();
                    
                     for (auto j = dep.begin(); j != dep.end();j++){
                        for (int k = 0; k < returned[j->first].size(); j++)
                        {
                          if (returned[j->first][k].is_string())
                              jvalue[(j->second).as_string()] = returned[j->first][k];
                           else if((j->second).is_string())
                              jvalue[(j->second).as_string()] = json::value(returned[j->first][k][(j->second).as_string()].serialize());
                           else
                              jvalue[(j->second).serialize()] = json::value(returned[j->first][k][(j->second).serialize()].serialize());

                           cout << jvalue.serialize() << endl;
                           if (!make_request(client, mtd, path, jvalue, json::value(), no++)){
                              cout<< "req failed" << endl;
                              return false;
                           }
                        }
                        
                     }
                  }
                  cout << "jvalue:" << jvalue.serialize() << endl;
                }
             }
             catch (http_exception const &e)
             {
                wcout << e.what() << endl;
             }
          })
          .wait();
   }
   uri_builder builder(path);
   cout << "path:" << path << endl;
   if (!jvalue.as_object().empty())
   {
      to_query(jvalue, &builder);
   } 
   string toURL = (s == NULL)?builder.to_string():(*s);
   cout << "Request to " << toURL << endl;
   make_task_request(client, mtd, toURL.c_str())
       .then([](http_response response) {
          // cout<<"RESPONSe:"<<response.to_string()<<endl;
          cout << "response:" << response.status_code();
          if (response.status_code() == status_codes::OK)
          {
             cout << " OK" << endl;
             return response.extract_json();
          }
          cout << "Not OK" << endl;
          return pplx::task_from_result(json::value());
       })
       .then([&path, &valid, &no](pplx::task<json::value> previousTask) {
          try
          {
             cout << "tyasks" << endl;
             std::fstream fs;
             fs.open("../src/test/responses/" + string(path) + "-" + to_string(no) + ".json", std::fstream::out);
             fs << endl;
             fs << previousTask.get().serialize() << endl;
             valid = check_valid(previousTask.get(), path);
             fs.close();
          }
          catch (http_exception const &e)
          {         
             wcout << e.what() << endl;
          }
       });
   return valid;
}

/*
save requests in ./test/requests.json in format

{
   "path" : "",                        //name of server's path 
   "query": {                          // query options for HTTP method
      "query name 1": "query value 1", //e.g "n":"All+Files"
      "query name 2": "query value 2" 
      } 
   ("put": true ,),                    // exists if testing put method
   "dependant" : "browseTop",     // declare dependancy from another request
   "dependantQuery":[                  // define which responses will be used as query from dependant
      "query name 1",
      "query name 2"                   // e.g "id","f"
   ]                  
   
}

 */

int main()
{
   http_client client(U("http://localhost:13780"));
      signal(SIGUSR1, sigHandler);
   pause();
   std::fstream fs;
   fs.open("../src/test/requests.json", std::fstream::in);
  
   json::array req = json::value::parse(fs).as_array();
   fs.close();
   method t;

   for (int i = 0; i < req.size(); i++)
   {
      cout<<"path:"<<req[i]["path"].as_string()<<endl;
      // cout<<"query"<<req[i]["query"].serialize()<<endl;
      // cout <<(req[i]["query"].is_null()?"empty":req[i]["query"].serialize())<<endl;
      t = req[i].has_field("put") ? methods::PUT : methods::GET;
      cerr << t << endl;
      cerr << "Request to " << req[i]["path"].as_string() << endl
           << ((
                   make_request(client, t,
                                req[i]["path"].as_string().c_str(),
                                req[i]["query"], req[i], 0))
                   ? " ok"
                   : " not ok")
           << endl;
   }
   cout << "finished requests" << endl;
}
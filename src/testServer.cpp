#include <cpprest/http_client.h>
#include <cpprest/json.h>

 
using namespace web;
using namespace web::http;
using namespace web::http::client;
 
#include <iostream>
using namespace std;


pplx::task<http_response> make_task_request(http_client & client, method mtd, const char* path)
{
   return  client.request(mtd, path);
    
}

void to_query(json::value jvalue,uri_builder *builder){
   
    json::object query = jvalue.as_object();
    for (auto i = query.cbegin(); i != query.cend(); i++ ){
       builder->append_query(i->first,i->second.as_string());
    }  
}
bool check_valid(json::value response, const char* path){
   if (response.as_object().empty()){
      return false;
   }
   if(response.has_field("error")){
      return false;
   }
   std::fstream fs;
   // fs.open (strcat("./test/preOut/",path), std::fstream::in);
   // fs.close();
   return true;
}

 
bool make_request(
   http_client & client, 
   method mtd, const char* path,
   json::value & jvalue, json::value req)
{
   
   bool valid=false;
   string *s;
   //check if query needs info from another one
   if(req.has_field("dependant")){
      make_task_request(client, methods::GET, req["dependant"].as_string().c_str() )
      .then([](http_response response)
      { if (response.status_code() == status_codes::OK)
         {
            return response.extract_json();
         }
         return pplx::task_from_result(json::value());
      })
      .then([&jvalue,&req,&path,&s](pplx::task<json::value> previousTask)
      {
         try
         {
            json::value returned = previousTask.get();
            cout<<"change path"<<returned.serialize()<<endl;
           // string *s;

            if(returned.has_string_field("addr")){
               cout << "here"<<endl;
               s = new string(returned["addr"].as_string());
               cout << *s<<endl;
               path = s->c_str();
               cout<<"path:"<<path<<endl;
            }
            cout<<"path:"<<path<<endl;
            for(auto i = req["dependantQuery"].as_array().cbegin();
               i!=req["dependantQuery"].as_array().cend(); i++){
               jvalue[i->as_string()] = previousTask.get()[i->as_string()];
            }        
         }
         catch (http_exception const & e)
         {
            wcout << e.what() << endl;
         }
      })
      .wait();
   }
   uri_builder builder(path);

   if(!jvalue.as_object().empty()){
    to_query(jvalue, &builder);
   }
   cout<<jvalue.serialize()<<"-WHAT-"<<builder.to_string()<<endl;
  // cout<<"REQUEST:" <<builder.to_string();
   make_task_request(client, mtd, builder.to_string().c_str())
      .then([](http_response response)
      {
        cout<<"RESPONSe:"<<response.to_string()<<endl;
         if (response.status_code() == status_codes::OK)
         {
            return response.extract_json();
         }
         return pplx::task_from_result(json::value());
      })
      .then([&path,&valid](pplx::task<json::value> previousTask)
      {
         try
         {
            std::fstream fs;
            fs.open ("./test/responses/"+string(path)+".json", std::fstream::out);
            fs << previousTask.get().serialize();
            valid=check_valid(previousTask.get(),path);
            fs.close();
         }
         catch (http_exception const & e)
         {
            wcout << e.what() << endl;
         }
      })
      .wait();
      //delete(path);
      if(s!=NULL)
         delete(s);
      return valid;
}
 
/*
save requests in ./test/requests.json in format

{
   "path" : "",                        //name of server's path 
   "query": {                          // query options for HTTP method
      "query name1": "query value1",   //e.g "n":"All+Files"
      "query name2": "query value2" 
      } 
   ("put": true ,),                    // exists if testing put method
   "dependant" : "browseTop.html",     // declare dependancy from another request
   "dependantQuery":{                  // define which responses will be used as query from dependant

   }
}

 */

int main()
{
    http_client client(U("http://localhost:8081"));
    
    std::fstream fs;
    fs.open ("./test/requests.json", std::fstream::in);
    json::array req= json::value::parse(fs).as_array();
    fs.close();
    method t; 
    for(int i = 0; i<req.size();i++){
        // cout<<"path:"<<req[i]["path"].as_string()<<endl;
        // cout<<"query"<<req[i]["query"].serialize()<<endl;
      // cout <<(req[i]["query"].is_null()?"empty":req[i]["query"].serialize())<<endl;
       t = req[i].has_field("put")? methods::PUT : methods::GET;
       cerr<<t<<endl;
       cerr<<"Request to "<<req[i]["path"].as_string()
        <<((
        make_request(client, t,
            req[i]["path"].as_string().c_str(),
            req[i]["query"],req[i])
        )?" ok":" not ok")
         <<endl;
    }
    
}
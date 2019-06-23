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
   json::value const & jvalue)
{
   uri_builder builder(path);
   bool valid=false;

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
      return valid;
}
 
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
            req[i]["query"])
        )?" ok":" not ok")
         <<endl;
    }
    
}
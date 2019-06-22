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

const char * to_query(json::value jvalue){
    ostringstream fs;
    fs<<"?";
    json::object query = jvalue.as_object();
    for (auto i = query.cbegin(); i != query.cend(); i++ ){
        fs<<i->first<<"="<<i->second.as_string();
        if((i+1)!=query.cend())
            fs<<"&";
    }
    //cout<<fs.str()<<endl;
    return fs.str().c_str();
}

 
void make_request(
   http_client & client, 
   method mtd, const char* path,
   json::value const & jvalue)
{
   uri_builder builder(path);
   if(!jvalue.as_object().empty()){
    builder.set_query(to_query(jvalue),true);
   }
 
   make_task_request(client, mtd, builder.to_string().c_str())
      .then([](http_response response)
      {
         if (response.status_code() == status_codes::OK)
         {
            return response.extract_json();
         }
         return pplx::task_from_result(json::value());
      })
      .then([&path](pplx::task<json::value> previousTask)
      {
         try
         {
            std::fstream fs;
            fs.open ("./test/responses/"+string(path)+".json", std::fstream::out);
            fs << previousTask.get().serialize();
            fs.close();
         }
         catch (http_exception const & e)
         {
            wcout << e.what() << endl;
         }
      })
      .wait();
}
 
int main()
{
    http_client client(U("http://localhost:8081"));
   // make_request(client,methods::GET,utility::string_t("/sproject.html").c_str(),json::value() );
    
    std::fstream fs;
    fs.open ("./test/requests.json", std::fstream::in);
    json::array req= json::value::parse(fs).as_array();
    fs.close();
    for(int i = 0; i<req.size();i++){
        cout<<"path:"<<req[i]["path"].as_string()<<endl;
        cout<<"query"<<req[i]["query"].serialize()<<endl;
      //  <<(req[i]["query"].is_null()?"empty":req[i]["query"].as_string())<<endl;
        make_request(client,methods::GET,
            req[i]["path"].as_string().c_str(),
            req[i]["query"]);
    }
    
}
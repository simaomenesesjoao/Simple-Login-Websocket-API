#include <iostream>
#include <curl/curl.h>
#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/client.hpp>
#include <websocketpp/client.hpp>
#include <nlohmann/json.hpp>
#include <string>
#include <set>
#include <format>



size_t write_callback(char* contents, size_t size, size_t nmemb, std::string* userp) {
    // Writes contents into the string pointed at by userp. New memory is allocated
    // to the string automatically via the 'append' method
    size_t real_size = size*nmemb;
    userp->append(contents, real_size);
    return real_size;
}



class Authenticator{
public:
    Authenticator();    
    std::string login(std::string url, std::string username, std::string password);
    ~Authenticator();
};

Authenticator::Authenticator(){
    curl_global_init(CURL_GLOBAL_DEFAULT);
}

Authenticator::~Authenticator(){
    curl_global_cleanup();
}

std::string Authenticator::login(std::string url, std::string username, std::string password){

    CURL* curl = curl_easy_init();
    std::string readBuffer;
    std::string token = "";

    if(curl == NULL){
        throw std::runtime_error("Could not initialize curl\n");
    }

    
    // Define the URL
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

    // Make this a POST request
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    
    // Set the POST fields for login. curl_easy_setopt doesn't require input sanitizing
    std::string postFields = std::format("username={}&password={}", username, password);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postFields.c_str());

    // Set up the callback function to handle the response data
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);


    // Send out the curl request
    CURLcode res = curl_easy_perform(curl);
    if(res != CURLE_OK) {
        throw std::runtime_error(curl_easy_strerror(res));
    } 

    // Parse the JSON response
    try {

        auto json = nlohmann::json::parse(readBuffer);

        if(json.contains("status")){

            auto status = json["status"];
            if(status == "ok" && json.contains("data")){
                token = json["data"];

            }
        }


    } catch (const std::exception& e) {
        std::cerr << "Failed to parse JSON: " << e.what() << std::endl;
    }


    curl_easy_cleanup(curl);

    if(token==""){
        throw std::runtime_error("Failed to login. Please check your credentials.\n");            
    }

    return token;   
}

typedef websocketpp::client<websocketpp::config::asio_tls_client> client;
typedef websocketpp::lib::shared_ptr<boost::asio::ssl::context> context_ptr;

using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;

typedef websocketpp::config::asio_tls_client::message_type::ptr message_ptr;


void on_message(client* c, std::set<std::string> *competiton_names, websocketpp::connection_hdl hdl, message_ptr msg) {
    // Websocket handler for when a message is received. It does two jobs:
    // 1. Finds competition names from the message and appends to a set
    // 2. If it finds a "sync" message, it close the websocket

    try{
        // Read the message from the websocket and parse it as json
        std::string message = msg->get_payload();
        nlohmann::json json = nlohmann::json::parse(message);

        // The json file structure that the websocket outputs is explained in the docs
        // {
        //    ts": timestamp,
        //    data": [
        //       [ code1, payload1 ],
        //       [ code2, payload2 ],
        //       ...
        //    ]
        // }
    
        if(json.contains("data")){
            for(auto& data_entries: json["data"]){
                std::string code = data_entries[0];
                nlohmann::json payload = data_entries[1];

                if(code == "event" && payload.contains("competition_name")){
                    std::string competition_name = payload["competition_name"];
                    competiton_names->insert(competition_name);
                }

                if(code == "sync"){
                    c->close(hdl, websocketpp::close::status::normal, "Client closing connection"); 
                    break;
                }
            }
        }

    } catch (const std::exception& e) {
        std::cerr << "Failed to parse JSON message: " << e.what() << std::endl;
    }


}


context_ptr on_tls_init(websocketpp::connection_hdl hdl) {
    // We need TSL support in order to connect to wss. This handle takes care of it

    context_ptr ctx(new boost::asio::ssl::context(boost::asio::ssl::context::tlsv12_client));
    ctx->set_options(boost::asio::ssl::context::default_workarounds |
        boost::asio::ssl::context::no_sslv2 | boost::asio::ssl::context::single_dh_use);

    return ctx;
}




class Websocket{
    // This class takes care of all the things required to setup a websocket connection

private:

    std::set<std::string> competiton_names;
    client ws_client;
    client::connection_ptr con;

public:

    Websocket(){

        // Disable logging 
        ws_client.clear_access_channels(websocketpp::log::alevel::all);
        ws_client.clear_error_channels(websocketpp::log::elevel::all);

        // Initialize ASIO
        ws_client.init_asio();

        // Register handlers for each relevant event
        ws_client.set_tls_init_handler(bind(&on_tls_init,::_1));
        ws_client.set_fail_handler([](websocketpp::connection_hdl hdl){std::cout << "Error: Connection failed";});
        ws_client.set_message_handler(bind(&on_message,&ws_client, &competiton_names,::_1,::_2));
        
        
    }

    void connect(std::string url){
        // Create a connection to the given URI and queue it for connection once
        // the event loop starts

        websocketpp::lib::error_code ec;
        con = ws_client.get_connection(url, ec);
        

        
        if (ec) 
            throw std::runtime_error("Failed to create websocket connection." + ec.message() + "\n");            
        

        ws_client.connect(con);

    }

    std::set<std::string> get_messages_til_sync(){
        // Runs the websocket until the message handler finds a "sync" event
        ws_client.run();
        return competiton_names;
    }



};


int main() {
    std::cout << "-- Simple Login + Websocket API --\n";

    std::string url = "https://api.mollybet.com/v1/sessions/";
    std::string username = "devinterview";
    std::string password = "OwAb6wrocirEv";


    Authenticator authenticator;
    std::string token = authenticator.login(url, username, password);
    std::cout << "[Info] Login successful.\n";

    std::string websocket_url = "wss://api.mollybet.com/v1/stream/?token=" + token;
    
    Websocket webs;
    webs.connect(websocket_url);
    std::cout << "[Info] Connected to websocket.\n";


    std::set<std::string> competiton_names = webs.get_messages_til_sync();
    std::cout << "[Info] Websocket closed.\n";
    
    
    std::cout << "[Info] Printing competition names:\n";
    for(const auto& name: competiton_names){
        std::cout << name << "\n";
    }
    return 0;
}

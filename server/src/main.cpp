#include <iostream>
#include <cpprest/http_listener.h>
#include <cpprest/json.h>

using namespace web;
using namespace web::http;
using namespace web::http::experimental::listener;

class http_server {
private:
    http_listener listener;
    
public:
    http_server(const std::string& url) : listener(utility::conversions::to_string_t(url)) {
        listener.support(methods::GET, std::bind(&http_server::handle_get, this, std::placeholders::_1));
        listener.support(methods::POST, std::bind(&http_server::handle_post, this, std::placeholders::_1));
        listener.support(methods::PUT, std::bind(&http_server::handle_put, this, std::placeholders::_1));
        listener.support(methods::DEL, std::bind(&http_server::handle_delete, this, std::placeholders::_1));
    }
    
    pplx::task<void> open() {
        return listener.open();
    }
    
    pplx::task<void> close() {
        return listener.close();
    }
    
private:
    void handle_get(http_request message) {
        std::wcout << L"GET request received\n";
        
        json::value response = json::value::object();
        response[U("status")] = json::value::string(U("GET request processed"));
        
        message.reply(status_codes::OK, response);
    }
    
    void handle_post(http_request message) {
        std::wcout << L"POST request received\n";
        
        message.extract_json()
            .then([&message](pplx::task<json::value> task) {
                try {
                    auto json_data = task.get();
                    
                    json::value response = json::value::object();
                    response[U("status")] = json::value::string(U("POST request processed"));
                    message.reply(status_codes::Created, response);
                }
                catch (const http_exception& e) {
                    message.reply(status_codes::BadRequest);
                }
            });
    }
    
    void handle_put(http_request message) {
        std::wcout << L"PUT request received\n";
        message.reply(status_codes::OK);
    }
    
    void handle_delete(http_request message) {
        std::wcout << L"DELETE request received\n";
        message.reply(status_codes::OK);
    }
};

int main() {
    std::cout << "Starting server...\n";
    
    try {
        http_server server("http://localhost:8080");
        server.open().wait();
        
        std::cout << "Server running on http://localhost:8080\n";
        std::cout << "Press Enter to exit...\n";
        std::cin.get();
        
        server.close().wait();
    }
    catch (const std::exception& e) {
        std::cout << "Error: " << e.what() << std::endl;
    }
}
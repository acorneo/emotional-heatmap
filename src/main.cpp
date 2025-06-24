#include "json.hpp"
#include <algorithm>
#include <chrono>
#include <cpprest/filestream.h>
#include <cpprest/http_client.h>
#include <cpprest/http_listener.h>
#include <cpprest/http_msg.h>
#include <cpprest/json.h>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

using namespace web;
using namespace web::http;
using namespace web::http::experimental::listener;
namespace fs = std::filesystem;

// in this DEQUE:
// pair < ip_addr, minutes after last request >
// in parallel thread i run a task that every minute removes 1 from each int and
// removes elements that have <= 0 cooldown left and that would work optimized
// because of how deque is implemented in c++, cool language btw, use it!!
std::deque<std::pair<std::string, int>> cooldown;

std::string get_content_type(const std::string &path) {
    std::string ext = fs::path(path).extension().string();
    if (ext == ".html" || ext == ".htm")
        return "text/html";
    if (ext == ".css")
        return "text/css";
    if (ext == ".js")
        return "application/javascript";
    if (ext == ".json")
        return "application/json";
    if (ext == ".png")
        return "image/png";
    if (ext == ".jpg" || ext == ".jpeg")
        return "image/jpeg";
    if (ext == ".svg")
        return "image/svg+xml";
    return "application/octet-stream"; // Default binary type
}
void handle_static_files(http_request request) {
    auto path = request.relative_uri().path();

    fs::path server_dir = fs::current_path();
    fs::path client_dir = server_dir.parent_path() / "client";

    std::cout << "Requested path: " << path << std::endl;

    fs::path file_path;
    if (path == "/" || path.empty()) {
        file_path = client_dir / "index.html";
    } else {
        std::string path_str = path;
        if (!path_str.empty() && path_str[0] == '/') {
            path_str = path_str.substr(1);
        }
        file_path = client_dir / path_str;
    }

    std::cout << "Looking for file: " << file_path << std::endl;

    if (fs::exists(file_path) && fs::is_regular_file(file_path)) {
        std::ifstream file(file_path, std::ios::binary);
        std::vector<unsigned char> file_data(
            (std::istreambuf_iterator<char>(file)),
            std::istreambuf_iterator<char>());

        http_response response(status_codes::OK);
        response.headers().add(header_names::content_type,
                               get_content_type(file_path.string()));
        response.set_body(file_data);
        request.reply(response);
    } else {
        request.reply(status_codes::NotFound, "File not found");
    }
}
std::string get_country_from_ip(const std::string &ip) {
    return "US"; // just for debugging

    try {
        web::http::client::http_client client(U("https://ipapi.co/"));
        uri_builder builder(U("/"));
        builder.append(ip);
        builder.append(U("country_name/"));

        http_response response =
            client.request(methods::GET, builder.to_string()).get();

        if (response.status_code() == status_codes::OK) {
            return response.extract_string().get();
        } else {
            std::cerr << "Error: HTTP " << response.status_code() << std::endl;
        }
    } catch (const std::exception &e) {
        std::cerr << "Exception in get_country_from_ip: " << e.what()
                  << std::endl;
    }
    return "Unknown";
}
void handle_mood_submit(http_request request) {
    auto headers = request.headers();
    std::string client_ip;

    try {
        client_ip = request.remote_address();
    } catch (std::exception const &e) {
        client_ip = "Unknown";
    }

    auto it = std::find_if(cooldown.begin(), cooldown.end(),
                           [&client_ip](const auto &p) {
                               if (p.first == client_ip) {
                                   return true;
                               }
                               return false;
                           });

    if (it != cooldown.end()) {
        auto answer = json::value::object();
        answer["message"] = json::value::string("You are on a cooldown.");
        request.reply(status_codes::TooManyRequests, answer);
        return;
    } else {
        cooldown.push_back(make_pair(
            client_ip, static_cast<int>(std::chrono::system_clock::to_time_t(
                           std::chrono::system_clock::now()))));
    }

    // get country and update data.json
    std::string country = get_country_from_ip(client_ip);
    fs::path dataPath = fs::current_path().parent_path() / "data.json";
    // std::cout << "Data file path: " << dataPath.string() << std::endl;
    // std::cout << "Current working directory: " << fs::current_path().string()
    //           << std::endl;

    nlohmann::json j;
    if (fs::exists(dataPath)) {
        try {
            std::ifstream in(dataPath);
            if (!in.is_open()) {
                std::cerr << "Failed to open data.json for reading"
                          << std::endl;
            } else {
                in >> j;
                std::cout << "File exists and was read successfully"
                          << std::endl;
                in.close();
            }
        } catch (const std::exception &e) {
            std::cerr << "Exception reading JSON: " << e.what() << std::endl;
            j = nlohmann::json::array();
        }
    } else {
        std::cout << "data.json doesn't exist, creating new array" << std::endl;
        j = nlohmann::json::array();
    }

    try {
        web::json::value json_data = request.extract_json().get();
        int mood = json_data["mood"].as_integer();
        if (mood < 0 || mood > 5) {
            auto answer = json::value::object();
            answer["message"] =
                json::value::string("Seems like you are doing some hacking on "
                                    "client-side... How about NO?");
            request.reply(status_codes::BadRequest, answer);
            return;
        }
        bool found = false;
        for (auto &entry : j) {
            if (entry["country"] == country) {
                entry["mood"] = entry["mood"].get<int>() + mood;
                entry["count"] = entry["count"].get<int>() + 1;
                found = true;
                break;
            }
        }
        if (!found) {
            nlohmann::json newObj;
            newObj["country"] = country;
            newObj["mood"] = mood;
            newObj["count"] = 1;
            j.push_back(newObj);
        }

    } catch (const web::http::http_exception &e) {
        std::cerr << "Error parsing JSON:" << e.what() << std::endl;
        auto answer = json::value::object();
        answer["message"] = json::value::string(
            "Could not proceed your request. Error with parsing.");
        request.reply(status_codes::BadRequest, answer);
        return;
    }

    std::ofstream out(dataPath);
    out << j.dump(2);
    out.close();

    auto answer = json::value::object();
    answer["message"] = json::value::string("Mood submitted successfully!");
    request.reply(status_codes::OK, answer);
}
void handle_ping(http_request request) {
    auto answer = json::value::object();
    answer["message"] = json::value::string("pong!");
    request.reply(status_codes::OK, answer);
}
void update_cooldowns() {
    int now = static_cast<int>(
        std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()));
    if (cooldown.empty())
        return;
    while (now - cooldown.front().second >= 300) {
        cooldown.pop_front();
        if (cooldown.empty()) {
            return;
        }
    }
    std::cout << "I updated cooldowns!\n";
    std::cout << "For first IP-addr: " << cooldown.front().first
              << " the diff in time is " << now - cooldown.front().second
              << std::endl;
}

void handle_data_req(http_request request) {
    fs::path dataPath = fs::current_path().parent_path() / "data.json";
    if (fs::exists(dataPath)) {
        try {
            std::ifstream in(dataPath);
            if (!in.is_open()) {
                std::cerr << "Failed to open data.json for reading"
                          << std::endl;
                request.reply(status_codes::InternalError,
                              "Error reading data file");
                return;
            }

            nlohmann::json j;
            in >> j;
            in.close();

            std::string json_str = j.dump();
            web::json::value response_json = web::json::value::parse(json_str);
            http_response response(status_codes::OK);
            response.headers().add(header_names::content_type,
                                   "application/json");
            response.set_body(response_json);
            request.reply(response);

        } catch (const std::exception &e) {
            std::cerr << "Exception reading/parsing data.json: " << e.what()
                      << std::endl;
            request.reply(status_codes::InternalError,
                          "Error processing data file");
        }
    } else {
        web::json::value empty_array = web::json::value::array();
        request.reply(status_codes::OK, empty_array);
    }
}

int main() {
    std::cout << "Starting server...\n";

    utility::string_t url = U("http://localhost:5252");
    http_listener listener(url);

    listener.support(methods::GET, [](http_request request) {
        auto path = request.relative_uri().path();
        if (path == "/api/ping") {
            handle_ping(request);
        } else if (path == "/api/request") {
            handle_data_req(request);
        } else {
            handle_static_files(request);
        }
    });

    listener.support(methods::POST, [](http_request request) {
        auto path = request.relative_uri().path();
        if (path == "/api/submit") {
            handle_mood_submit(request);
        }
    });

    std::thread([] {
        while (true) {
            update_cooldowns();
            std::this_thread::sleep_for(std::chrono::seconds(30));
        }
    }).detach();

    try {
        listener.open()
            .then([&listener]() {
                std::cout << "\nlistening on http://localhost:5252\n";
            })
            .wait();

        std::cout << "Server is running. The client directory is being served."
                  << std::endl;

        // Keep server running
        for (;;)
            ;
    } catch (std::exception const &e) {
        std::cout << e.what() << std::endl;
    }
    return 0;
}
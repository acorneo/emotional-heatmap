cmake_minimum_required(VERSION 3.9)
project(emotional_heatmap LANGUAGES CXX)

find_package(cpprestsdk REQUIRED)
find_package(OpenSSL REQUIRED) 

add_executable(${PROJECT_NAME} src/main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE cpprestsdk::cpprest
    OpenSSL::Crypto
    OpenSSL::SSL)

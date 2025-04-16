##
## EPITECH PROJECT, 2025
## CMakeLists.txt
## File description:
## it makes files
##

cmake_minimum_required(VERSION 3.10)

project(network_lib LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

set(HEADERS
        include/network/tcp_client_connection.hpp
        include/network/tcp_server.hpp
        include/network/tcp_server_connection.hpp
        include/network/tcp_socket.hpp
)

add_library(${PROJECT_NAME} INTERFACE)

target_include_directories(${PROJECT_NAME} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

##
## EPITECH PROJECT, 2025
## CMakeLists.txt
## File description:
## it makes files
##


cmake_minimum_required(VERSION 3.10)
project(jetpack)

add_subdirectory(network_lib)
add_subdirectory(common)

add_subdirectory(client)
add_subdirectory(server)

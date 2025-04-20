## EPITECH PROJECT, 2025
## CMakeLists.txt
## File description:
## it makes files
##

cmake_minimum_required(VERSION 3.10.0)

project(jetpack_client VERSION 1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SOURCES
        src/main.cpp
)

set(HEADERS
        include/input_manager.hpp
        include/window_manager.hpp
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

set_target_properties(${PROJECT_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/$<CONFIG>
)

target_include_directories(${PROJECT_NAME} PRIVATE include)

target_compile_options(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Debug>:-g -O0 -Wall -Wextra>
        $<$<CONFIG:Release>:-O3 -DNDEBUG>
)

target_link_libraries(${PROJECT_NAME} PRIVATE network_lib common)

set(SFML_DIR "/opt/homebrew/Cellar/sfml/3.0.0/lib/cmake/SFML")
find_package(SFML 3 COMPONENTS Graphics Window System REQUIRED)

target_link_libraries(${PROJECT_NAME} PRIVATE SFML::Graphics SFML::Window SFML::System)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E cmake_echo_color --yellow --bold
        "🎉 Executable ${PROJECT_NAME} placed in: ${CMAKE_SOURCE_DIR}/bin/$<CONFIG>"
)

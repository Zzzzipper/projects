cmake_minimum_required(VERSION 3.9)

project(http-server-post VERSION 1.0.1 DESCRIPTION "Http server with POST request paser")

include(GNUInstallDirs)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

set(CLIENT_SOURCE "${CMAKE_SOURCE_DIR}/tests/http-srv-post/main.cpp")

add_executable(http-server-post ${CLIENT_SOURCE})

target_link_libraries(http-server-post
    boost_system
    boost_program_options
    boost_log
    pthread
    boost_thread)

cmake_minimum_required(VERSION 3.9)

project(client VERSION 1.0.1 DESCRIPTION "Simulating receiver client app ")

include(GNUInstallDirs)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

set(CLIENT_SOURCE "${CMAKE_SOURCE_DIR}/tests/client/main.cpp"
    "${CMAKE_SOURCE_DIR}/tests/client/test.cpp"
    "${CMAKE_SOURCE_DIR}/src/log.cpp")

add_definitions(-DBOOST_LOG_DYN_LINK)

add_executable(client ${CLIENT_SOURCE})

target_link_libraries(client
    boost_system
    boost_program_options
    boost_log
    boost_log_setup
    pthread
    boost_thread)

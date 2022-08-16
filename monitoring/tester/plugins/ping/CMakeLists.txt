cmake_minimum_required(VERSION 3.9)

project(ping VERSION 1.0.1 DESCRIPTION "Plugin dynamic loaded library (PING)")

include(GNUInstallDirs)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

include_directories(${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/plugins/include
    .)

add_library(ping SHARED
    ../plugin.cpp
    ../plugin_api.cpp
    engine.cpp
    ${CMAKE_SOURCE_DIR}/src/protocol.cpp
    ${CMAKE_SOURCE_DIR}/src/json.cpp)

#add_executable(ping
#    ../plugin.cpp
#    ../api.cpp
#    engine.cpp
#    ${CMAKE_SOURCE_DIR}/src/protocol.cpp
#    ${CMAKE_SOURCE_DIR}/src/json.cpp
#    ${CMAKE_SOURCE_DIR}/src/log.cpp)

#target_link_libraries(ping
#    boost_filesystem
#    boost_regex
#    boost_system
#    boost_log
#    pthread
#    boost_thread
#    boost_program_options
#    dl
#    ${OPENSSL_LIBRARIES})

add_definitions(-DLOG_TIME
    -DLOG_LEVEL
    -DENABLE_LOGGGING)

set_target_properties(ping PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
    PUBLIC_HEADER ../../include/plugin.h)

target_include_directories(ping PRIVATE .)

install(TARGETS ping
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})


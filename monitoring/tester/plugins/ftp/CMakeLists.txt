cmake_minimum_required(VERSION 3.9)

project(ftp VERSION 1.0.1 DESCRIPTION "Plugin dynamic loaded library (FTP)")

include(GNUInstallDirs)

find_package(OpenSSL REQUIRED)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

include_directories(${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/plugins/include
    .)

add_library(ftp SHARED
    client/reply.cpp
    client/request.cpp
    client/session.cpp

    ../plugin.cpp
    ../plugin_api.cpp
    engine.cpp
    ${CMAKE_SOURCE_DIR}/src/protocol.cpp
    ${CMAKE_SOURCE_DIR}/src/json.cpp)

#add_executable(ftp
#    client/reply.cpp
#    client/request.cpp
#    client/session.cpp

#    ../plugin.cpp
#    ../api.cpp
#    engine.cpp
#    ${CMAKE_SOURCE_DIR}/src/protocol.cpp
#    ${CMAKE_SOURCE_DIR}/src/json.cpp
#    ${CMAKE_SOURCE_DIR}/src/log.cpp)

#target_link_libraries(ftp
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

set_target_properties(ftp PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
    PUBLIC_HEADER ../../include/plugin.h)

target_include_directories(ftp PRIVATE .)

install(TARGETS ftp
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

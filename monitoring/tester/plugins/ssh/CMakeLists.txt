cmake_minimum_required(VERSION 3.9)

project(ssh VERSION 1.0.1 DESCRIPTION "Plugin dynamic loaded library (SSH)")

include(GNUInstallDirs)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

find_package(OpenSSL REQUIRED)

include_directories(${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/plugins/include
    .)

add_library(ssh SHARED
    ../plugin.cpp
    ../plugin_api.cpp
    engine.cpp
    ${CMAKE_SOURCE_DIR}/src/protocol.cpp
    ${CMAKE_SOURCE_DIR}/src/json.cpp)

add_library(pssh SHARED ../plugin.cpp engine.cpp)

set_target_properties(pssh PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
    PUBLIC_HEADER ../../include/plugin.h)

target_include_directories(pssh PRIVATE .)
target_link_libraries(pssh ssh)

install(TARGETS pssh
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

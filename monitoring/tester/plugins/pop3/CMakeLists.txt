cmake_minimum_required(VERSION 3.9)

project(pop3 VERSION 1.0.1 DESCRIPTION "Plugin dynamic loaded library (POP3)")

include(GNUInstallDirs)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

add_library(pop3 SHARED ../plugin.cpp engine.cpp)

set_target_properties(pop3 PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
    PUBLIC_HEADER ../../include/plugin.h)

target_include_directories(pop3 PRIVATE .)

install(TARGETS pop3
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})


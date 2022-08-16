cmake_minimum_required(VERSION 3.9)

project(port-scan VERSION 1.0.1 DESCRIPTION "Plugin dynamic loaded library (Port-Scan)")

include(GNUInstallDirs)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

add_library(port-scan SHARED ../plugin.cpp engine.cpp)

set_target_properties(port-scan PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
    PUBLIC_HEADER ../../include/plugin.h)

target_include_directories(port-scan PRIVATE .)

install(TARGETS port-scan
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})


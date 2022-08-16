cmake_minimum_required(VERSION 3.9)

project(ldd VERSION 1.0.1 DESCRIPTION "Class to make dynamic library loads simple and easy")

include(GNUInstallDirs)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

add_library(ldd SHARED ldd_library_manager.cpp ldd_library.cpp)

set_target_properties(ldd PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
    PUBLIC_HEADER ldd.h)

# configure_file(mylib.pc.in mylib.pc @ONLY)

target_include_directories(ldd PRIVATE .)

install(TARGETS ldd
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# install(FILES ${CMAKE_BINARY_DIR}/mylib.pc
#     DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig)

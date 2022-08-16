cmake_minimum_required(VERSION 3.9)

project(httpsrv VERSION 1.0.1 DESCRIPTION "Http 1.0 server module")

include(GNUInstallDirs)

set(HTTPSRV_SOURCE "${CMAKE_SOURCE_DIR}${SET_REFERENCE_TESTER}/modules/http/connection.cpp"
    "${CMAKE_SOURCE_DIR}${SET_REFERENCE_TESTER}/modules/http/connection_manager.cpp"
#    "${CMAKE_SOURCE_DIR}/modules/http/main.cpp"
    "${CMAKE_SOURCE_DIR}${SET_REFERENCE_TESTER}/modules/http/mime_types.cpp"
    "${CMAKE_SOURCE_DIR}${SET_REFERENCE_TESTER}/modules/http/reply.cpp"
    "${CMAKE_SOURCE_DIR}${SET_REFERENCE_TESTER}/modules/http/request_handler.cpp"
    "${CMAKE_SOURCE_DIR}${SET_REFERENCE_TESTER}/modules/http/request_parser.cpp"
    "${CMAKE_SOURCE_DIR}${SET_REFERENCE_TESTER}/modules/http/server.cpp"
    "${CMAKE_SOURCE_DIR}${SET_REFERENCE_TESTER}/modules/http/command_dispatch.cpp")

# if(NOT ALM)
#     set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/ubuntu_bin)
# else()
#     set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/alm_bin)
# endif(NOT ALM)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)


#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

add_definitions(-DBOOST_ASIO_HAS_MOVE -DLOG_TIME -DLOG_LEVEL -DENABLE_LOGGGING)

add_library(httpsrv SHARED ${HTTPSRV_SOURCE})
#add_executable(httpsrv ${HTTPSRV_SOURCE})
#target_link_libraries(httpsrv
#    boost_system
#    boost_log
#    pthread
#    boost_thread
#    boost_program_options
#    dl)

set_target_properties(httpsrv PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
    PUBLIC_HEADER server.h)

# configure_file(mylib.pc.in mylib.pc @ONLY)

target_include_directories(httpsrv PRIVATE .)

install(TARGETS httpsrv
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# install(FILES ${CMAKE_BINARY_DIR}/mylib.pc
#     DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig)


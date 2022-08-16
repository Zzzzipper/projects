cmake_minimum_required(VERSION 3.5.1)

project(tester VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT ALM)
    find_package( Boost 1.71 COMPONENTS program_options REQUIRED )
else()
    find_package( Boost 1.66 COMPONENTS program_options REQUIRED )
endif(NOT ALM)
find_package(OpenSSL REQUIRED)

include_directories( ${Boost_INCLUDE_DIR} )


# configure_file("${CMAKE_SOURCE_DIR}/include/ver.h.in" "${CMAKE_SOURCE_DIR}/include/ver.h")

include_directories(${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/modules/http
    ${CMAKE_SOURCE_DIR}/modules/ldd
    ${OpenSSL_INCLUDE_DIR})

# add_subdirectory(nagios)
add_subdirectory(modules/ldd)
add_subdirectory(modules/http)
add_subdirectory(plugins/ping)
# add_subdirectory(plugins/db-connect)
# add_subdirectory(plugins/dns)
# add_subdirectory(plugins/domain)
if(NOT ALM)
    add_subdirectory(plugins/ftp)
endif(NOT ALM)
#add_subdirectory(plugins/http)
#add_subdirectory(plugins/http-head)
# add_subdirectory(plugins/imap)
# add_subdirectory(plugins/pop3)
# add_subdirectory(plugins/port-scan)
# add_subdirectory(plugins/smtp)
add_subdirectory(plugins/ssh)
# add_subdirectory(tests/client)
# add_subdirectory(tests/http-srv-post)

set(SERVER_SOURCE "${CMAKE_SOURCE_DIR}/src/main.cpp"
    "${CMAKE_SOURCE_DIR}/src/dispatcher.cpp"
    "${CMAKE_SOURCE_DIR}/src/tcpsrv.cpp"
    "${CMAKE_SOURCE_DIR}/src/requestor.cpp"
    "${CMAKE_SOURCE_DIR}/src/sender.cpp"
    "${CMAKE_SOURCE_DIR}/src/config.cpp"
    "${CMAKE_SOURCE_DIR}/src/curve25519-donna-c64.c"
    "${CMAKE_SOURCE_DIR}/src/aes.cpp"
    "${CMAKE_SOURCE_DIR}/src/protocol.cpp"
    "${CMAKE_SOURCE_DIR}/src/json.cpp"
    "${CMAKE_SOURCE_DIR}/src/handshake.cpp"
    "${CMAKE_SOURCE_DIR}/src/log.cpp"
    "${CMAKE_SOURCE_DIR}/src/holder.cpp"
    "${CMAKE_SOURCE_DIR}/src/timer.cpp"
    "${CMAKE_SOURCE_DIR}/src/system.cpp"
    "${CMAKE_SOURCE_DIR}/src/worker.cpp"
    "${CMAKE_SOURCE_DIR}/src/utils.cpp")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

add_executable(tester ${SERVER_SOURCE})

add_definitions(-DBOOST_ASIO_HAS_MOVE -DBOOST_LOG_DYN_LINK -DLOG_TIME -DLOG_LEVEL -DENABLE_LOGGGING)
add_compile_options(-Wno-dev)

target_link_libraries(tester
    boost_system
    boost_filesystem
    pthread
    boost_thread
    boost_program_options
    boost_regex
    dl
    httpsrv
    ldd
    z
    ${OPENSSL_LIBRARIES})

# The first use create include/build.h and not use BUILD_NUMBER.
# After first build BUILD_NUMBER is enabled to compile with includes build.h
#add_custom_command(
#    TARGET tester
#    PRE_BUILD
#    COMMAND ${CMAKE_COMMAND} -DDIR=${CMAKE_SOURCE_DIR}
#        -P "${CMAKE_SOURCE_DIR}/version.cmake"
#)

add_custom_command(
    TARGET tester
    PRE_BUILD
    COMMAND ${CMAKE_SOURCE_DIR}/stamp.sh ${CMAKE_SOURCE_DIR} include
)

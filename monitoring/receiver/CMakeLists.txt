cmake_minimum_required(VERSION 3.5.1)

project(receiver VERSION 1.0.1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Boost 1.66 COMPONENTS program_options REQUIRED)

find_package(OpenSSL REQUIRED)

include_directories( ${Boost_INCLUDE_DIR} )

# configure_file("${CMAKE_SOURCE_DIR}/tester/include/ver.h.in" "${CMAKE_SOURCE_DIR}/tester/include/ver.h")

include_directories(${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/tester/include
    ${CMAKE_SOURCE_DIR}/tester/modules/http
    ${CMAKE_SOURCE_DIR}/tester/modules/ldd
    ${OpenSSL_INCLUDE_DIR})

set(SET_REFERENCE_TESTER /tester)
# add_subdirectory(tester/modules/ldd)
add_subdirectory(tester/modules/http)
# add_subdirectory(tests/client)
# add_subdirectory(tests/http-srv-post)

set(SERVER_SOURCE "${CMAKE_SOURCE_DIR}/src/main.cpp"
    "${CMAKE_SOURCE_DIR}/tester/src/dispatcher.cpp"
    "${CMAKE_SOURCE_DIR}/tester/src/tcpsrv.cpp"
    "${CMAKE_SOURCE_DIR}/tester/src/requestor.cpp"
    "${CMAKE_SOURCE_DIR}/tester/src/sender.cpp"
    "${CMAKE_SOURCE_DIR}/tester/src/config.cpp"
    "${CMAKE_SOURCE_DIR}/tester/src/curve25519-donna-c64.c"
    "${CMAKE_SOURCE_DIR}/tester/src/aes.cpp"
    "${CMAKE_SOURCE_DIR}/tester/src/protocol.cpp"
    "${CMAKE_SOURCE_DIR}/tester/src/json.cpp"
    "${CMAKE_SOURCE_DIR}/tester/src/handshake.cpp"
    "${CMAKE_SOURCE_DIR}/tester/src/log.cpp"
    "${CMAKE_SOURCE_DIR}/tester/src/timer.cpp"
    "${CMAKE_SOURCE_DIR}/tester/src/system.cpp"
#    "${CMAKE_SOURCE_DIR}/tester/src/holder.cpp"
    "${CMAKE_SOURCE_DIR}/tester/src/utils.cpp"
    "${CMAKE_SOURCE_DIR}/src/dblink.cpp"
    "${CMAKE_SOURCE_DIR}/src/tester.cpp"
    "${CMAKE_SOURCE_DIR}/src/dataflow.cpp")

if(NOT ALM)
     add_definitions(-DBLOB_PQ)
#     set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/ubuntu_bin)
#else()
#     set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/alm_bin)
endif(NOT ALM)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

add_executable(receiver ${SERVER_SOURCE})

add_definitions(-DBOOST_ASIO_HAS_MOVE
    -DBOOST_LOG_DYN_LINK
    -DLOG_TIME
    -DLOG_LEVEL
    -DENABLE_LOGGGING
    -DRECEIVER_SIDE)

add_compile_options(-Wno-dev)

target_link_libraries(receiver
    boost_system
    boost_filesystem
    pthread
    boost_thread
    boost_program_options
    boost_regex
    dl
    httpsrv
    z
    pqxx
    stdc++fs
    ${OPENSSL_LIBRARIES})

#add_custom_command(
#    TARGET receiver
#    PRE_BUILD
#    COMMAND ${CMAKE_COMMAND} -DDIR=${CMAKE_SOURCE_DIR}
#        -P "${CMAKE_SOURCE_DIR}/version.cmake"
#)

add_custom_command(
    TARGET receiver
    PRE_BUILD
    COMMAND ${CMAKE_SOURCE_DIR}/stamp.sh ${CMAKE_SOURCE_DIR} include
)

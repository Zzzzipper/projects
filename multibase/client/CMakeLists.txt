cmake_minimum_required(VERSION 3.5)

project(multicli LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

FIND_PACKAGE( Boost 1.71 COMPONENTS program_options REQUIRED )
INCLUDE_DIRECTORIES( ${Boost_INCLUDE_DIR} )

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../bin)

include_directories(${CMAKE_SOURCE_DIR}/../server/inc)

add_executable(client main.cpp test.cpp)
target_link_libraries(client boost_system boost_program_options boost_log pthread boost_thread)

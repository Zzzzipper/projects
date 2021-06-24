cmake_minimum_required(VERSION 2.8)

project(remoteq)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../bin)

add_definitions(-DNCHANNELS=3)
add_executable(${PROJECT_NAME} src/server.cpp src/udsserver.cpp src/uds.cpp src/loguru.cpp src/handlers.cpp src/message.cpp src/headmeters.cpp)
target_link_libraries(${PROJECT_NAME} PUBLIC pthread dl)
target_include_directories(${PROJECT_NAME} PUBLIC
                          "${PROJECT_BINARY_DIR}"
                          "${PROJECT_SOURCE_DIR}/include"
                          )

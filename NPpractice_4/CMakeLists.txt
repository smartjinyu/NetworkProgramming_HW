cmake_minimum_required(VERSION 3.6)
project(NPpractice_4)

set(CMAKE_CXX_STANDARD 11)

set(BUILD_1 server)
set(SOURCE_FILES_1 server.cpp)
add_executable(Server ${SOURCE_FILES_1})

set(BUILD_2 client)
set(SOURCE_FILES_2 client.cpp)
add_executable(Client ${SOURCE_FILES_2})
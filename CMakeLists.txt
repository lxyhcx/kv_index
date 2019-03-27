project(kv_index)
cmake_minimum_required(VERSION 2.8)
add_definitions(-std=c++11)
add_definitions("-Wall -g")

aux_source_directory(src SRC_LIST)
aux_source_directory(test SRC_LIST)
include_directories(include)

add_executable(${PROJECT_NAME} ${SRC_LIST})

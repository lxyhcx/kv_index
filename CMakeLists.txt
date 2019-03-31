project(kv_index)
cmake_minimum_required(VERSION 2.8)

add_definitions("-std=c++11 -Wall -g")

aux_source_directory(src SRC_LIST)
aux_source_directory(test TEST_LIST)

include_directories(include)

add_library(${PROJECT_NAME} SHARED ${SRC_LIST})

add_executable(test_index test/test_index.cpp)
add_executable(test_db test/test_db.cpp)

target_link_libraries(test_index ${PROJECT_NAME})
target_link_libraries(test_db ${PROJECT_NAME})

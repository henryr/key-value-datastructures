cmake_minimum_required(VERSION 3.1)

set(CMAKE_CXX_STANDARD 14)

include_directories(SYSTEM ${CMAKE_SOURCE_DIR}/thirdparty/gtest-1.7.0/include)
link_directories(${CMAKE_SOURCE_DIR}/thirdparty/gtest-1.7.0/)

add_executable(btree-test btree.cc)
target_link_libraries(btree-test gtest pthread)
target_compile_options(btree-test PRIVATE -g)

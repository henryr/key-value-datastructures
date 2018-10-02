cmake_minimum_required(VERSION 3.1)

set(CMAKE_CXX_STANDARD 14)

include_directories(SYSTEM ${CMAKE_SOURCE_DIR}/thirdparty/gtest-1.8.0/include)
link_directories(${CMAKE_SOURCE_DIR}/thirdparty/gtest-1.8.0/lib/)

include_directories(SYSTEM ${CMAKE_SOURCE_DIR}/thirdparty/google-benchmark/include)
link_directories(${CMAKE_SOURCE_DIR}/thirdparty/google-benchmark/lib/)

add_library(formica
  formica/index.cc
  formica/circular-log.cc)
target_compile_options(formica PRIVATE -g -O3)

add_executable(formica-test formica/formica-test.cc)
target_link_libraries(formica-test formica gtest pthread)
target_compile_options(formica-test PRIVATE -g -O3)

add_executable(btree-test btree-tests.cc btree.cc)
target_link_libraries(btree-test gtest pthread)
target_compile_options(btree-test PRIVATE -g -O3)
# target_compile_options(btree-test PRIVATE -fsanitize=address)
# target_link_libraries(btree-test -fsanitize=address)

add_executable(formica-benchmark formica/formica-benchmark.cc)
target_link_libraries(formica-benchmark formica benchmark)
target_compile_options(formica-benchmark PRIVATE -g -O3)

# target_compile_options(formica-benchmark PRIVATE -fsanitize=address)
# target_link_libraries(formica-benchmark -fsanitize=address)

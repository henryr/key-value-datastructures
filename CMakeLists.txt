cmake_minimum_required(VERSION 3.1)

set(CMAKE_CXX_STANDARD 14)

include_directories(SYSTEM ${CMAKE_SOURCE_DIR}/thirdparty/gtest-1.8.0/include)
link_directories(${CMAKE_SOURCE_DIR}/thirdparty/gtest-1.8.0/lib/)

include_directories(SYSTEM ${CMAKE_SOURCE_DIR}/thirdparty/google-benchmark/include)
link_directories(${CMAKE_SOURCE_DIR}/thirdparty/google-benchmark/lib/)

add_executable(mica-test mica/mica-test.cc mica/mica-log.cc)
target_link_libraries(mica-test gtest pthread)
target_compile_options(mica-test PRIVATE -g -O3)

add_executable(btree-test btree-tests.cc btree.cc)
target_link_libraries(btree-test gtest pthread)
target_compile_options(btree-test PRIVATE -g -O3)
# target_compile_options(btree-test PRIVATE -fsanitize=address)
# target_link_libraries(btree-test -fsanitize=address)

add_executable(mica-benchmark mica/mica-benchmark.cc mica/mica-log.cc)
target_link_libraries(mica-benchmark benchmark)
target_compile_options(mica-benchmark PRIVATE -g -O3)

# target_compile_options(mica-benchmark PRIVATE -fsanitize=address)
# target_link_libraries(mica-benchmark -fsanitize=address)

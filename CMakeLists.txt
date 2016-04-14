
cmake_minimum_required(VERSION 3.1)
project(ThreadPool)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -pthread -ggdb -g3 -O0")

set(SRCS 
    main.cpp)



include_directories("${source_dir}/include")

add_executable(ThreadPool ${SRCS})


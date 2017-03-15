cmake_minimum_required(VERSION 3.6)
project(source)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES
    main.cpp
    similarity.cpp
    similarity.hpp)

add_executable(source ${SOURCE_FILES})
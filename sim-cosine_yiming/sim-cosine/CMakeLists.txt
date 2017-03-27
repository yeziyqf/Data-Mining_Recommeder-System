cmake_minimum_required(VERSION 3.6)
project(sim_cosine)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES
    source/main.cpp
    source/similarity.cpp
    source/similarity.hpp)

add_executable(sim_cosine ${SOURCE_FILES})
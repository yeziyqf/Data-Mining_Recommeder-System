cmake_minimum_required(VERSION 3.6)
project(danielbernardes_socialfiltering_861ac2e07b11)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES
    sf/source/graph-util.c
    sf/source/graph.c
    sf/source/graph.h
    sf/source/main.cpp
    sf/source/socialfiltering.cpp
    sf/source/socialfiltering.hpp
    sf/source/wgraph.c
    sf/source/wgraph.h
    sim-cosine/source/main.cpp
    sim-cosine/source/similarity.cpp
    sim-cosine/source/similarity.hpp
    sim-jaccard/source/main.cpp
    sim-jaccard/source/similarity.cpp
    sim-jaccard/source/similarity.hpp)

add_executable(danielbernardes_socialfiltering_861ac2e07b11 ${SOURCE_FILES})
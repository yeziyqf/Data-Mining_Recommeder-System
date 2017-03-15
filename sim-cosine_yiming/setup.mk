#
# Social Filtering dependencies and definitions
#

# Setup the include/lib flags variables with the path of Boost and Eigen. E.g.:
#
# INCLUDE_FLAGS = -I../include 
# LIB_FLAGS     = -L../lib

INCLUDE_FLAGS =-I /usr/include 
LIB_FLAGS     = -L /usr/lib64

PREFIX = ../bin
SOURCE = source

# General definitions
CXX      = g++
CCFLAGS  = -O3
CXXFLAGS = -O3 -std=c++11
CPPFLAGS = $(INCLUDE_FLAGS)
LDFLAGS  = $(LIB_FLAGS)
LDLIBS   = -lboost_program_options
WDEBUG   = -Wall -Wextra -DNDEBUG


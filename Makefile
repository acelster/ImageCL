# Copyright (c) 2016, Thomas L. Falch
# For conditions of distribution and use, see the accompanying LICENSE and README files

# This file is part of the ImageCL source-to-source compiler
# developed at the Norwegian University of Science and technology

ROSE_DIR=

ROSE_INCLUDE_DIR=$(ROSE_DIR)/build/include/rose
ROSE_LIB_DIR=$(ROSE_DIR)/build/lib
ROSE_STATIC_LIB_DIR=$(ROSE_DIR)/build_static/lib
BOOST_LIB_DIR=$(ROSE_DIR)/boost/installTree/lib
JAVA_LIB_DIR=/usr/lib/jvm/java-7-openjdk-amd64/jre/lib/amd64/server

CPPFLAGS=-std=c++11 -g -I $(ROSE_INCLUDE_DIR) -I src

SOURCES_ALL = $(wildcard src/*.cpp)
SOURCES = $(filter-out src/main.cpp, $(SOURCES_ALL))
OBJ_FILES = $(patsubst %.cpp, %.o, $(SOURCES))


all: iclc

iclc : src/main.cpp $(OBJ_FILES)
	g++ -std=c++11 src/main.cpp $(OBJ_FILES) -g -I $(ROSE_INCLUDE_DIR) -L $(ROSE_LIB_DIR) -L $(BOOST_LIB_DIR) -L $(JAVA_LIB_DIR)  -l rose -l boost_system -l boost_iostreams -l jvm -l boost_date_time -l boost_thread -l boost_filesystem -l boost_program_options -l boost_regex -l boost_wave -o iclc

clean:
	rm -f clite tester src/*.o unittests/*.o

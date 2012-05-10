# Copyright (c) 2011 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

#
# GNU Make based build file.  For details on GNU Make see:
#   http://www.gnu.org/software/make/manual/make.html
#

#
# Project information
#
# These variables store project specific settings for the project name
# build flags, files to copy or install.  In the examples it is typically
# only the list of sources and project name that will actually change and
# the rest of the makefile is boilerplate for defining build rules.
#
PROJECT:=nacl
LDFLAGS:=-lppapi -lppapi_gles2

#-lppapi_cpp

CXX_SOURCES:= $(wildcard *.cpp)
CXX_SOURCES:= $(CXX_SOURCES) $(wildcard *.cc)

#C_SOURCES:=$(wildcard ../assimp/contrib/zlib/*.c)
#C_SOURCES:=$(C_SOURCES) $(wildcard ../assimp/contrib/unzip/*.c)
#C_SOURCES:=$(C_SOURCES) $(wildcard ../assimp/contrib/ConvertUTF/*.c)

#
# Get pepper directory for toolchain and includes.
#
# If PEPPER_ROOT is not set, then assume it can be found a two directories up,
# from the default example directory location.
#
THIS_MAKEFILE:=$(abspath $(lastword $(MAKEFILE_LIST)))
NACL_SDK_ROOT?=$(abspath $(dir $(THIS_MAKEFILE))../..)

#-Werror
# Project Build flags
WARNINGS:=-Wno-long-long -Wall -Wswitch-enum -pedantic
CXXFLAGS:=-pthread -std=gnu++98 $(WARNINGS) -I$(NACL_SDK_ROOT)/toolchain/$(OSNAME)_x86_glibc/x86_64-nacl/include
#CXXFLAGS:=$(CXXFLAGS) -I../assimp/include -I../assimp/code/BoostWorkaround -DASSIMP_BUILD_BOOST_WORKAROUND
#CXXFLAGS:=$(CXXFLAGS) -I../jpeg

#CFLAGS:=-pthread -std=gnu89 $(WARNINGS) -I$(NACL_SDK_ROOT)/toolchain/$(OSNAME)_x86_glibc/x86_64-nacl/include
#CFLAGS:=$(CFLAGS) -I../assimp/include -I../assimp/code/BoostWorkaround -DASSIMP_BUILD_BOOST_WORKAROUND
#CFLAGS:=$(CFLAGS) -I../jpeg

#
# Compute tool paths
#
#
OSNAME:=$(shell python $(NACL_SDK_ROOT)/tools/getos.py)
TC_PATH:=$(abspath $(NACL_SDK_ROOT)/toolchain/$(OSNAME)_x86_glibc)
CXX:=$(TC_PATH)/bin/i686-nacl-g++

# Alias for C compiler
CC:=$(TC_PATH)/bin/i686-nacl-gcc

#
# Disable DOS PATH warning when using Cygwin based tools Windows
#
CYGWIN ?= nodosfilewarning
export CYGWIN


# Declare the ALL target first, to make the 'all' target the default build
all: $(PROJECT)_x86_32.nexe $(PROJECT)_x86_64.nexe $(PROJECT).nmf

# Define 32 bit compile and link rules for C sources
#Cx86_32_OBJS:=$(patsubst %.c,%_32.o,$(C_SOURCES))
#$(Cx86_32_OBJS) : %_32.o : %.c $(THIS_MAKE)
#	$(CC) -o $@ -c $< -m32 -O0 -g $(CFLAGS)

# Define 32 bit compile and link rules for C++ sources
x86_32_OBJS:=$(patsubst %.cpp,%_32.o,$(CXX_SOURCES))
$(x86_32_OBJS) : %_32.o : %.cpp $(THIS_MAKE)
	$(CXX) -o $@ -c $< -m32 -O0 -g $(CXXFLAGS)

$(PROJECT)_x86_32.nexe : $(x86_32_OBJS) $(Cx86_32_OBJS)
	$(CXX) -o $@ $^ -m32 -O0 -g $(CXXFLAGS) $(LDFLAGS)

# Define 64 bit compile and link rules for C sources
#Cx86_64_OBJS:=$(patsubst %.c,%_64.o,$(C_SOURCES))
#$(Cx86_64_OBJS) : %_64.o : %.c $(THIS_MAKE)
#	$(CC) -o $@ -c $< -m64 -O0 -g $(CFLAGS)
	
# Define 64 bit compile and link rules for C++ sources
x86_64_OBJS:=$(patsubst %.cpp,%_64.o,$(CXX_SOURCES))
$(x86_64_OBJS) : %_64.o : %.cpp $(THIS_MAKE)
	$(CXX) -o $@ -c $< -m64 -O0 -g $(CXXFLAGS)

$(PROJECT)_x86_64.nexe : $(x86_64_OBJS)
	$(CXX) -o $@ $^ -m64 -O0 -g $(CXXFLAGS) $(LDFLAGS)

#
# NMF Manifiest generation
#
# Use the python script create_nmf to scan the binaries for dependencies using
# objdump.  Pass in the (-L) paths to the default library toolchains so that we
# can find those libraries and have it automatically copy the files (-s) to
# the target directory for us.
NMF:=python $(NACL_SDK_ROOT)/tools/create_nmf.py
NMF_ARGS:=-D $(TC_PATH)/x86_64-nacl/bin/objdump
NMF_PATHS:=-L $(TC_PATH)/x86_64-nacl/lib32 -L $(TC_PATH)/x86_64-nacl/lib

$(PROJECT).nmf : $(PROJECT)_x86_64.nexe $(PROJECT)_x86_32.nexe
	echo $(NMF) $(NMF_ARGS) -s . -o $@ $(NMF_PATHS) $^
	$(NMF) $(NMF_ARGS) -s . -o $@ $(NMF_PATHS) $^

# Define a phony rule so it always runs, to build nexe and start up server.
.PHONY: RUN 
RUN: all
	python ../httpd.py



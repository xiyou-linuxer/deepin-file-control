#!/bin/sh

PLATFORM=$(uname -s)
CXXFLAGS=-std=c++11

#if [ $PLATFORM == "Linux" ]; then
    CXX=g++
    EXTRA="Poll.cc"
#elif [ $PLATFORM == "Darwin" ]; then
#    CXX=clang++
#    EXTRA=Poll.cc
#else
#    echo "unknown platform"
#    exit 1
#fi

SRC="Buffer.cc Channel.cc EventLoop.cc Coder.cc Logger.cc
     Socket.cc Timer.cc Server.cc $EXTRA"

echo "$CXX $CXXFLAGS $SRC -lpthread"

$CXX $CXXFLAGS $SRC -lpthread

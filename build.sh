#!/bin/sh

mkdir -p bin
g++ -D_DEBUG -Wall -g -O0 -Iether/include -o bin/ether ether/ether.cpp && \
bin/ether res/hello_world.eth    # for now we run the compiler here

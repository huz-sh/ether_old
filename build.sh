#!/bin/sh

mkdir -p bin
gcc -D_DEBUG -Wall -g -O0 -Iether/include -o bin/ether ether/ether.c && \
bin/ether ether/ether.c    # for now we run the compiler here

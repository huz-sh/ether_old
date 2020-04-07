#!/bin/sh

mkdir -p bin
gcc -D_DEBUG -Wall -Wextra -Wshadow -std=c99 -m64 -g -O0 -Iether/include -o bin/ether ether/*.c && \
bin/ether res/function_test.eth


#!/bin/sh

mkdir -p bin
gcc -Wall -g -O0 -o bin/ether ether/ether.c
bin/ether    # for now we run the compiler

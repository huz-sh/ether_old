#!/bin/sh

mkdir -p bin
nasm -felf64 -o bin/ether_stdlib.o ether_stdlib/ether_stdlib.asm	&& \
gcc -D_DEBUG -Wall -Wextra -Wshadow -std=c99 -m64 -g -O0 -Iether/include -o bin/ether ether/*.c && \
bin/ether .tmp/hello.eth && \
ld -o bin/a.out bin/ether_stdlib.o .tmp/hello.o && \
echo "-------------" && \
bin/a.out



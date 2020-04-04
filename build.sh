#!/bin/sh

mkdir -p bin
gcc -D_DEBUG -Wall -Wextra -g -O0 -Iether/include -o bin/ether \
	ether/ether.c \
	ether/ether_buf.c \
	ether/ether_error.c \
	ether/ether_io.c \
	ether/ether_lexer.c \
	ether/ether_parser.c \
	ether/ether_str_intern.c \
	&& \
bin/ether res/expr_test.eth    # for now we run the compiler here

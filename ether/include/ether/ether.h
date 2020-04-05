#ifndef __ETHER_H
#define __ETHER_H

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#define true 1
#define false 0
#define null NULL

typedef unsigned int uint;
typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef unsigned char uchar;

#define MIN(x, y) ((x) <= (y) ? (x) : (y))
#define MAX(x, y) ((x) >= (y) ? (x) : (y))
#define CLAMP_MAX(x, max) MIN(x, max)
#define CLAMP_MIN(x, min) MAX(x, min)

typedef struct {
	size_t len;
	size_t cap;
	char buf[];
} buf_hdr;

#define buf__hdr(b) ((buf_hdr*)((char*)(b) - offsetof(buf_hdr, buf)))

#define buf_len(b) ((b) ? buf__hdr(b)->len : 0)
#define buf_cap(b) ((b) ? buf__hdr(b)->cap : 0)
#define buf_end(b) ((b) + buf_len(b))
#define buf_sizeof(b) ((b) ? buf_len(b) * sizeof(*b) : 0)

#define buf_free(b)        ((b) ? (free(buf__hdr(b)), (b) = null) : 0)
#define buf_fit(b, n)      ((n) <= buf_cap(b) ? 0 :						\
							((b) = buf__grow((b), (n), sizeof(*(b)))))
#define buf_push(b, ...)   (buf_fit((b), 1 + buf_len(b)),				\
							(b)[buf__hdr(b)->len++] = (__VA_ARGS__))
#define buf_printf(b, ...) ((b) = buf__printf((b), __VA_ARGS__))
#define buf_clear(b)       ((b) ? buf__hdr(b)->len = 0 : 0)

void* buf__grow(const void* buf, size_t new_len, size_t elem_size);

typedef struct {
	char* fpath;
	char* contents;
	uint len;
} file;

file ether_read_file(char* fpath);
char* get_line_at(file f, uint64 l);

int print_file_line(file f, uint64 l);
int print_file_line_with_info(file f, uint64 l);

int print_marker_arrow_ln(file f, uint64 l, uint32 c);
int print_marker_arrow_with_info_ln(file f, uint64 l, uint32 c);

void ether_error(const char* fmt, ...);

typedef struct {
	size_t len;
	char* str;
} intern;

char* strni(char* start, char* end);
char* stri(char* str);

#define ETHER_ERROR true
#define ETHER_SUCCESS false

#define LEXER_ERROR_COUNT_MAX 10

typedef enum {
	TOKEN_L_BKT, 
	TOKEN_R_BKT,
	TOKEN_COLON,
	TOKEN_PLUS,
	TOKEN_MINUS,
	TOKEN_STAR,
	TOKEN_SLASH,
	TOKEN_EQUAL,
	
	TOKEN_IDENTIFIER,
	TOKEN_NUMBER,
	TOKEN_STRING,
	
	TOKEN_SCOPE,
} token_type;

typedef struct {
	token_type type;
	char* lexeme;
	uint64 line;
	uint32 col;
} token;

void lexer_init(file);
token** lexer_run(int*);

typedef enum {
	EXPR_BINARY,
	EXPR_NUMBER,
	EXPR_VARIABLE,
} expr_type;

typedef struct expr expr;

typedef struct {
	expr* left;
	expr* right;
	token* op;
} expr_binary;

struct expr {
	expr_type type;
	union {
		expr_binary binary;
		token* number;
		token* variable;
	};
};

typedef struct {
	token* type;
	uint8 npointer;
} data_type;

typedef struct stmt stmt;

typedef enum {
	STMT_VAR_DECL,
} stmt_type;

typedef struct {
	data_type* type;
	token* identifier;
	expr* initializer;
} stmt_var_decl;

struct stmt {
	stmt_type type;
	union {
		stmt_var_decl var_decl;
	};
};

void parser_init(file, token**);
stmt** parser_run(int*);

#ifdef _DEBUG
#define print_ast(e) _print_ast(e)
#else
#define print_ast(e)
#endif

void _print_ast(expr* e);

#endif

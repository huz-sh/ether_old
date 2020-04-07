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
	TOKEN_L_BKT = 0, 
	TOKEN_R_BKT,
	TOKEN_COLON,
	TOKEN_PLUS,
	TOKEN_MINUS,
	TOKEN_STAR,
	TOKEN_SLASH,
	TOKEN_EQUAL,
	TOKEN_COMMA, 
	
	TOKEN_IDENTIFIER,
	TOKEN_KEYWORD,
	TOKEN_NUMBER,
	TOKEN_STRING,
	
	TOKEN_SCOPE,

	TOKEN_EOF,
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
	EXPR_NUMBER,
	EXPR_VARIABLE,
	EXPR_FUNC_CALL,
} expr_type;

typedef struct expr expr;

typedef struct {
	token* callee;
	expr** args;
} func_call;

struct expr {
	expr_type type;
	union {
		func_call func_call;
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
	STMT_STRUCT,
	STMT_FUNC,
	STMT_VAR_DECL,
	STMT_EXPR,
} stmt_type;

typedef struct {
	data_type* type;
	token* identifier;
} struct_field; 

typedef struct {
	token* identifier;
	struct_field** fields;	
} stmt_struct;

typedef struct {
	data_type* type;
	token* identifier;
} param;

typedef struct {
	data_type* type;
	token* identifier;
	param* params;
	stmt** body;
} stmt_func;

typedef struct {
	data_type* type;
	token* identifier;
	expr* initializer;
} stmt_var_decl;

struct stmt {
	stmt_type type;
	union {
		stmt_struct _struct;
		stmt_func func;
		stmt_var_decl var_decl;
		expr* expr_stmt;
	};
};

void parser_init(file, token**);
stmt** parser_run(int*);

#ifdef _DEBUG
#define print_ast(e) _print_ast(e)
#else
#define print_ast(e)
#endif

void _print_ast(stmt** e);

#endif

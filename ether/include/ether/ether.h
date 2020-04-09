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

typedef unsigned int  uint;
typedef unsigned char uchar;
typedef signed char schar;

typedef uint8_t	 u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t	i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef schar bool;
typedef int error_code;

#define MIN(x, y) ((x) <= (y) ? (x) : (y))
#define MAX(x, y) ((x) >= (y) ? (x) : (y))
#define CLAMP_MAX(x, max) MIN(x, max)
#define CLAMP_MIN(x, min) MAX(x, min)

typedef struct {
	u64 len;
	u64 cap;
	char buf[];
} BufHdr;

#define buf__hdr(b) ((BufHdr*)((char*)(b) - offsetof(BufHdr, buf)))

#define buf_len(b) ((b) ? buf__hdr(b)->len : 0)
#define buf_cap(b) ((b) ? buf__hdr(b)->cap : 0)
#define buf_end(b) ((b) + buf_len(b))
#define buf_sizeof(b) ((b) ? buf_len(b) * sizeof(*b) : 0)

#define buf_free(b)		   ((b) ? (free(buf__hdr(b)), (b) = null) : 0)
#define buf_fit(b, n)	   ((n) <= buf_cap(b) ? 0 :							\
							((b) = buf__grow((b), (n), sizeof(*(b)))))
#define buf_push(b, ...)   (buf_fit((b), 1 + buf_len(b)),				\
							(b)[buf__hdr(b)->len++] = (__VA_ARGS__))
#define buf_printf(b, ...) ((b) = buf__printf((b), __VA_ARGS__))
#define buf_clear(b)	   ((b) ? buf__hdr(b)->len = 0 : 0)

void* buf__grow(const void* buf, u64 new_len, u64 elem_size);

typedef struct {
	char* fpath;
	char* contents;
	uint len;
} SourceFile;

SourceFile* ether_read_file(char* fpath);
char* get_line_at(SourceFile* file, u64 line);

error_code print_file_line(SourceFile* file, u64 line);
error_code print_file_line_with_info(SourceFile* file, u64 line);

error_code print_marker_arrow_ln(SourceFile* file, u64 line, u32 column);
error_code print_marker_arrow_with_info_ln(SourceFile* file,
										   u64 line, u32 column);

void ether_error(const char* fmt, ...);

typedef struct {
	u64 len;
	char* str;
} Intern;

char* str_intern_range(char* start, char* end);
char* str_intern(char* str);

#define ETHER_ERROR true
#define ETHER_SUCCESS false

#define LEXER_ERROR_COUNT_MAX 10
/* TODO: parser error count max */
/* TODO: linker error count max */

typedef enum {
	TOKEN_LEFT_BRACKET,
	TOKEN_RIGHT_BRACKET,
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

	TOKEN_EOF,
} TokenType;

typedef struct {
	TokenType type;
	char* lexeme;
	SourceFile* srcfile;
	u64 line;
	u32 column;
} Token;

void lexer_init(SourceFile* file);
Token** lexer_run(error_code* out_error_code);

typedef enum {
	EXPR_NUMBER,
	EXPR_VARIABLE,
	EXPR_FUNC_CALL,
} ExprType;

typedef struct Expr Expr;

typedef struct {
	Token* callee;
	Expr** args;
} FuncCall;

struct Expr {
	ExprType type;
	union {
		FuncCall func_call;
		Token* number;
		Token* variable;
	};
};

typedef struct {
	Token* type;
	u8 pointer_count;
} DataType;

typedef struct Stmt Stmt;

typedef enum {
	STMT_STRUCT,
	STMT_FUNC,
	STMT_VAR_DECL,
	STMT_EXPR,
} StmtType;

typedef struct {
	DataType* type;
	Token* identifier;
} Field;

typedef struct {
	Token* identifier;
	Field** fields;
} Struct;

typedef struct {
	DataType* type;
	Token* identifier;
} Param;

typedef struct {
	DataType* type;
	Token* identifier;
	Param** params;
	Stmt** body;
} Func;

typedef struct {
	DataType* type;
	Token* identifier;
	Expr* initializer;
	bool is_global_var;
} VarDecl;

struct Stmt {
	StmtType type;
	union {
		Struct struct_stmt;
		Func func;
		VarDecl var_decl;
		Expr* expr;
	};
};

void parser_init(Token** tokens_buf);
Stmt** parser_run(error_code* out_error_code);

#ifdef _DEBUG
#define print_ast(stmts) print_ast_debug(stmts)
#else
#define print_ast(stmts)
#endif

void print_ast_debug(Stmt** stmts);

typedef struct Scope {
	Stmt** variables;
	struct Scope* parent_scope;
} Scope;

void linker_init(Stmt*** stmts_buf);
error_code linker_run(void);

#endif

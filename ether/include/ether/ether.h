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

#define TAB_SIZE 4

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
#define buf_fit(b, n)	   ((n) <= buf_cap(b) ? 0 :	\
							((b) = buf__grow((b), (n), sizeof(*(b)))))
#define buf_push(b, ...)   (buf_fit((b), 1 + buf_len(b)), \
							(b)[buf__hdr(b)->len++] = (__VA_ARGS__))
#define buf_pop(b)		   ((b) ? buf__shrink((b), 1) : 0)
#define buf_printf(b, ...) ((b) = buf__printf((b), __VA_ARGS__))
#define buf_clear(b)	   ((b) ? buf__hdr(b)->len = 0 : 0)

void* buf__grow(const void* buf, u64 new_len, u64 elem_size);
void buf__shrink(const void* buf, u64 size);

typedef char echar;

echar* estr_create(char* str);
u64 estr_len(echar* estr);
void estr_append(echar* dest, char* src);
u64 estr_find_last_of(echar* estr, char c);
echar* estr_sub(echar* estr, u64 start, u64 end);

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
	TOKEN_PERCENT,
	TOKEN_EQUAL,
	TOKEN_LESS,
	TOKEN_LESS_EQUAL,
	TOKEN_GREATER,
	TOKEN_GREATER_EQUAL,
	TOKEN_COMMA,
	TOKEN_DOT,

	TOKEN_IDENTIFIER,
	TOKEN_KEYWORD,
	TOKEN_NUMBER,
	TOKEN_CHAR,
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

typedef struct {
	SourceFile* srcfile;
	Token** tokens;
	char** keywords;

	char* start, *cur;
	u64 line;
	char* last_newline;
	char* last_to_last_newline;

	uint error_count;
	error_code error_occured;
} Lexer;

bool is_token_identical(Token* a, Token* b);

Token** lexer_run(Lexer* lexer, SourceFile* file, error_code* out_error_code);

typedef enum {
	EXPR_NUMBER,
	EXPR_CHAR,
	EXPR_STRING,
	EXPR_NULL,
	EXPR_BOOL,
	EXPR_VARIABLE,
	EXPR_FUNC_CALL,
	EXPR_DOT_ACCESS,
} ExprType;

typedef struct Expr Expr;
typedef struct Stmt Stmt;

typedef struct {
	Token* callee;
	Expr** args;
	Stmt* function_called;
} FuncCall;

typedef struct {
	Token* identifier;
	Stmt* variable_decl_referenced;
} VariableRef;

typedef struct {
	Expr* left;
	Token* right;
	bool is_left_pointer;
} DotAccess;

struct Expr {
	ExprType type;
	Token* head;
	union {
		FuncCall func_call;
		VariableRef variable;
		DotAccess dot;		
		Token* number;
		Token* chr;
		Token* string;
		Token* boolean;
	};
};

typedef struct {
	Token* type;
	u8 pointer_count;
} DataType;

typedef enum {
	STMT_STRUCT,
	STMT_FUNC,
	STMT_VAR_DECL,
	STMT_IF,
	STMT_FOR,
	STMT_WHILE,
	STMT_RETURN,
	STMT_EXPR,
} StmtType;

typedef struct {
	DataType* type;
	Token* identifier;
	Stmt* struct_referenced;
} Field;

typedef struct {
	Token* identifier;
	Field** fields;
} Struct;

typedef struct {
	DataType* type;
	Token* identifier;
	Stmt** params; /* param is a statement to make it easier to add to scope */
	Stmt** body;
	bool is_function; /* false if decl */
	bool public;
} Func;

typedef struct {
	DataType* type;
	Token* identifier;
	Expr* initializer;
	bool is_global_var;
	bool is_variable; /* false if extern var */
} VarDecl;

typedef enum {
	IF_IF_BRANCH,
	IF_ELIF_BRANCH,
	IF_ELSE_BRANCH
} IfBranchType;

typedef struct {
	Expr* cond;
	Stmt** body;
} IfBranch;

typedef struct {
	IfBranch* if_branch;
	IfBranch** elif_branch;
	IfBranch* else_branch;
} If;

typedef struct {
	Stmt* counter;
	Expr* to;
	Stmt** body;
} For;

typedef struct {
	Expr* cond;
	Stmt** body;
} While;

typedef struct {
	Expr* expr;
	Stmt* function_referernced;
	Token* keyword;
} Return;

struct Stmt {
	StmtType type;
	union {
		Struct struct_stmt;
		Func func;
		VarDecl var_decl;
		If if_stmt;
		For for_stmt;
		While while_stmt;
		Return return_stmt;
		Expr* expr;
	};
};

typedef struct {
	Token** tokens;
	u64 tokens_len;
	Stmt** stmts;
	SourceFile* srcfile;

	char** built_in_data_types;
	char** operator_keywords;

	u64 idx;
	uint error_count;
	bool error_occured;
	uint error_bracket_counter;
	bool error_panic;
	bool start_stmt_bracket;
	Stmt* current_function;
} Parser;

Stmt** parser_run(Parser* parser, Token** tokens,
				  SourceFile* file, error_code* out_error_code);

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

void token_error(bool* error_occured, uint* error_count,
				 Token* t, const char* fmt, ...);
void token_warning(Token* t, const char* fmt, ...);
void token_note(Token* token, const char* fmt, ...);

void linker_init(Stmt** p_stmts);
Stmt** linker_run(error_code* err_code);

typedef struct {
	char* a;
	char* b;
} ImplicitCastTypeMap;

typedef struct {
	char* type;
	u64 size;
} TypeSizeMap;

void resolve_init(Stmt** p_stmts, Stmt** p_structs);
error_code resolve_run(void);

void code_gen_init(Stmt** p_stmts, SourceFile* p_srcfile);
void code_gen_run(void);

typedef struct {
	char* fpath;
	char* obj_fpath;
} Loader;

error_code loader_load(Loader* loader, char* fpath, char* obj_fpath);

#endif

#include <ether/ether.h>

static uint tab_count;

static void print_stmt(Stmt*);
static void print_stmt_with_newline(Stmt*);
static void print_struct(Stmt*);
static void print_func(Stmt*);
static void print_var_decl(Stmt*);
static void print_expr_stmt(Stmt*);

static void print_expr(Expr*);
static void print_number_expr(Expr*);
static void print_variable_expr(Expr*);
static void print_func_call(Expr*);

static void print_data_type(DataType*);
static void print_token(Token*);
static void print_tabs_by_indentation(void);

inline static void print_left_bracket(void);
inline static void print_right_bracket(void);
inline static void print_colon(void);
inline static void print_equal(void);
inline static void print_space(void);
inline static void print_newline(void);
inline static void print_tab(void);
inline static void print_string(const char*);
inline static void print_char(char);

void print_ast_debug(Stmt** stmts) {
	tab_count = 0;
	for (u64 i = 0; i < buf_len(stmts); ++i) {
		print_stmt_with_newline(stmts[i]);
	}
	print_newline();
}

static void print_stmt(Stmt* stmt) {
	print_tabs_by_indentation();
	if (stmt->type != STMT_EXPR) print_left_bracket();
	switch (stmt->type) {
		case STMT_STRUCT:	print_struct(stmt); break;
		case STMT_FUNC:			print_func(stmt); break;
		case STMT_VAR_DECL: print_var_decl(stmt); break;
		case STMT_EXPR:			print_expr_stmt(stmt); break;
	}
	if (stmt->type != STMT_EXPR) print_right_bracket();
}

static void print_stmt_with_newline(Stmt* stmt) {
	print_stmt(stmt);
	print_newline();
}

static void print_struct(Stmt* stmt) {
	print_string("struct ");
	print_token(stmt->struct_stmt.identifier);

	Field** fields = stmt->struct_stmt.fields;
	for (uint i = 0; i < buf_len(fields); ++i) {
		print_newline();
		print_tab();
		print_left_bracket();
		print_data_type(fields[i]->type);
		print_colon();
		print_space();
		print_token(fields[i]->identifier);
		print_right_bracket();
	}
}

static void print_func(Stmt* stmt) {
	print_string("define ");
	print_data_type(stmt->func.type);
	print_colon();
	print_space();
	print_token(stmt->func.identifier);
	print_space();

	print_left_bracket();
	Param** p = stmt->func.params;
	u64 params_len = buf_len(stmt->func.params);
	for (u64 i = 0; i < params_len; ++i) {
		print_data_type(p[i]->type);
		print_colon();
		print_token(p[i]->identifier);

		if (i < params_len - 1) {
			print_space();
		}
	}
	print_right_bracket();
	
	print_newline();
	tab_count++;
	u64 body_len = buf_len(stmt->func.body);
	for (u64 i = 0; i < body_len; ++i) {
		print_stmt(stmt->func.body[i]);
		if (i < (body_len - 1)) {
			print_newline();
		}
	}
	tab_count--;
}

static void print_var_decl(Stmt* stmt) {
	print_string("let ");
	print_data_type(stmt->var_decl.type);
	print_colon();
	print_token(stmt->var_decl.identifier);
	if (stmt->var_decl.initializer) {
		print_space();
		print_expr(stmt->var_decl.initializer);
	}
}

static void print_expr_stmt(Stmt* stmt) {
	print_expr(stmt->expr);
}

static void print_expr(Expr* expr) {
	switch (expr->type) {
		case EXPR_NUMBER: print_number_expr(expr); break;
		case EXPR_VARIABLE: print_variable_expr(expr); break;
		case EXPR_FUNC_CALL: print_func_call(expr); break;
	}
}

static void print_number_expr(Expr* expr) {
	print_string(expr->number->lexeme);
}

static void print_variable_expr(Expr* expr) {
	print_string(expr->variable->lexeme);
}

static void print_func_call(Expr* expr) {
	print_left_bracket();
	u64 args_len = buf_len(expr->func_call.args);
	print_token(expr->func_call.callee);

	if (args_len != 0) print_space();
	for (u64 i = 0; i < args_len; ++i) {
		print_expr(expr->func_call.args[i]);
		if (i < (args_len - 1)) {
			print_space();
		}
	}
	print_right_bracket();
}

static void print_data_type(DataType* data_type) {
	print_token(data_type->type);
	for (u8 i = 0; i < data_type->pointer_count; ++i) {
		print_char('*');
	}
}

static void print_tabs_by_indentation(void) {
	uint current_tab_counter = 0;

	while (current_tab_counter != tab_count) {
		print_tab();
		++current_tab_counter;
	}
}

inline static void print_token(Token* t) {
	assert(t);
	printf("%s", t->lexeme);
}

inline static void print_left_bracket(void) {
	print_char('[');
}

inline static void print_right_bracket(void) {
	print_char(']');
}

inline static void print_colon(void) {
	print_char(':');
}

inline static void print_equal(void) {
	print_char('=');
}

inline static void print_space(void) {
	print_char(' ');
}

inline static void print_newline(void) {
	print_char('\n');
}

inline static void print_tab(void) {
	print_string("	  ");
}

inline static void print_string(const char* string) {
	printf("%s", string);
}

inline static void print_char(char c) {
	printf("%c", c);
}

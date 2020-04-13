#include <ether/ether.h>

FILE* popen(const char*, const char*);
int pclose(FILE*);

static Stmt*** stmts_all;
static char* output_code;
static uint tab_count;

static void code_gen_destroy(void);

static void gen_struct_decls(void);
static void gen_struct_decl(Stmt*);
static void gen_structs(void);
static void gen_struct(Stmt*);
static void gen_inline_struct(Stmt*);
static void gen_field(Field*);
static void gen_global_var_decls(void);
static void gen_func_decls(void);
static void gen_func_decl(Stmt*);
static void gen_file(Stmt**);
static void gen_func(Stmt*);
static void gen_stmt(Stmt*);
static void gen_var_decl(Stmt*);
static void gen_if_stmt(Stmt*);
static void gen_expr_stmt(Stmt*);

static void gen_expr(Expr*);
static void gen_number_expr(Expr*);
static void gen_string_expr(Expr*);
static void gen_variable_expr(Expr*);
static void gen_func_call(Expr*);
static void gen_set_expr(Expr*);
static void gen_arithmetic_expr(Expr*);
static void gen_comparison_expr(Expr*);

static void print_data_type(DataType*);
static void print_token(Token*);
static void print_string(char*);
static void print_tabs_by_indentation(void);
static void print_semicolon(void);
static void print_left_paren(void);
static void print_right_paren(void);
static void print_tab(void);
static void print_space(void);
static void print_newline(void);
static void print_char(char);

static void compile_output_code(void);

void code_gen_init(Stmt*** stmts_buf) {
	stmts_all = stmts_buf;
	output_code = null;
	tab_count = 0;
}

void code_gen_run(void) {
	gen_struct_decls();
	gen_structs();
	gen_global_var_decls();
	gen_func_decls();
	
	for (u64 i = 0; i < buf_len(stmts_all); ++i) {
		gen_file(stmts_all[i]);
	}
	buf_push(output_code, '\0');
	printf("%s", output_code); /* TODO: remove this */

	compile_output_code();
	
	code_gen_destroy();
}

static void code_gen_destroy(void) {
	buf_free(output_code);
}

static void gen_struct_decls(void) {
	for (u64 file = 0; file < buf_len(stmts_all); ++file) {
		for (u64 stmt = 0; stmt < buf_len(stmts_all[file]); ++stmt) {
			Stmt* current_stmt = stmts_all[file][stmt];
			if (current_stmt->type == STMT_STRUCT) {
				gen_struct_decl(current_stmt);
			}
		}
	}
	print_newline();
}

static void gen_struct_decl(Stmt* stmt) {
	print_string("typedef struct ");
	print_token(stmt->struct_stmt.identifier);
	print_space();
	print_token(stmt->struct_stmt.identifier);
	print_semicolon();
	print_newline();
}

static void gen_structs(void) {
	for (u64 file = 0; file < buf_len(stmts_all); ++file) {
		for (u64 stmt = 0; stmt < buf_len(stmts_all[file]); ++stmt) {
			Stmt* current_stmt = stmts_all[file][stmt];
			if (current_stmt->type == STMT_STRUCT) {
				gen_struct(current_stmt);
			}
		}
	}
}

static void gen_struct(Stmt* stmt) {
	print_tabs_by_indentation();
	print_string("typedef struct ");
	print_token(stmt->struct_stmt.identifier);
	print_string(" {");
	print_newline();

	tab_count++;
	Field** fields = stmt->struct_stmt.fields;
	for (u64 i = 0; i < buf_len(fields); ++i) {
		print_tabs_by_indentation();
		gen_field(fields[i]);
	}
	tab_count--;
	print_string("} ");
	print_token(stmt->struct_stmt.identifier);
	print_semicolon();
	print_newline();
	print_newline();
}

static void gen_inline_struct(Stmt* stmt) {
	print_string("struct ");
	print_string("{");
	print_newline();

	tab_count++;
	Field** fields = stmt->struct_stmt.fields;
	for (u64 i = 0; i < buf_len(fields); ++i) {
		print_tabs_by_indentation();
		gen_field(fields[i]);
	}
	tab_count--;
	print_tabs_by_indentation();
	print_string("}");
}

static void gen_field(Field* field) {
	if (field->type->type->type == TOKEN_IDENTIFIER &&
		field->type->pointer_count == 0) {
		assert(field->struct_referenced);
		gen_inline_struct(field->struct_referenced);
	}
	else {
		print_data_type(field->type);
	}
	print_space();
	print_token(field->identifier);
	print_semicolon();
	print_newline();
}

static void gen_global_var_decls(void) {
	for (u64 file = 0; file < buf_len(stmts_all); ++file) {
		for (u64 stmt = 0; stmt < buf_len(stmts_all[file]); ++stmt) {
			Stmt* current_stmt = stmts_all[file][stmt];
			if (current_stmt->type == STMT_VAR_DECL) {
				gen_var_decl(current_stmt);
			}
		}
	}
	print_newline();
}

static void gen_func_decls(void) {
	for (u64 file = 0; file < buf_len(stmts_all); ++file) {
		for (u64 stmt = 0; stmt < buf_len(stmts_all[file]); ++stmt) {
			Stmt* current_stmt = stmts_all[file][stmt];
			if (current_stmt->type == STMT_FUNC) {
				gen_func_decl(current_stmt);
			}
		}
	}
	print_newline();
}

static void gen_func_decl(Stmt* stmt) {
	print_data_type(stmt->func.type);
	print_space();
	print_token(stmt->func.identifier);

	print_left_paren();
	Stmt** params = stmt->func.params;
	for (u64 i = 0; i < buf_len(params); ++i) {
		print_data_type(params[i]->var_decl.type);
		print_space();
		print_token(params[i]->var_decl.identifier);
		if (i != (buf_len(params) - 1)) {
			print_string(", ");
		}
	}
	print_right_paren();
	print_semicolon();
	print_newline();
}

static void gen_file(Stmt** stmts) {
	for (u64 i = 0; i < buf_len(stmts); ++i) {
		if (stmts[i]->type == STMT_FUNC) {
			gen_func(stmts[i]);
		}
	}	
}

static void gen_func(Stmt* stmt) {
}

static void gen_stmt(Stmt* stmt) {
	switch (stmt->type) {
		case STMT_VAR_DECL: gen_var_decl(stmt); break;
		case STMT_IF: gen_if_stmt(stmt); break;	
		case STMT_EXPR: gen_expr_stmt(stmt); break;
		case STMT_STRUCT:
		case STMT_FUNC: break;	
	}
}

static void gen_var_decl(Stmt* stmt) {
	print_data_type(stmt->var_decl.type);
	print_space();
	print_token(stmt->var_decl.identifier);
	if (stmt->var_decl.initializer) {
		print_string(" = ");
		gen_expr(stmt->var_decl.initializer);
	}
	print_semicolon();
	print_newline();
}

static void gen_if_stmt(Stmt* stmt) {
}

static void gen_expr_stmt(Stmt* stmt) {
}

static void gen_expr(Expr* expr) {
	switch (expr->type) {
		case EXPR_NUMBER: gen_number_expr(expr); break;
		case EXPR_STRING: gen_string_expr(expr); break;
		case EXPR_VARIABLE: gen_variable_expr(expr); break;
		case EXPR_FUNC_CALL: gen_func_call(expr); break;	
	}
}

static void gen_number_expr(Expr* expr) {
	print_token(expr->number);
}

static void gen_string_expr(Expr* expr) {
	print_token(expr->string);
}

static void gen_variable_expr(Expr* expr) {
	print_token(expr->variable.identifier);
}

static void gen_func_call(Expr* expr) {
	if (expr->func_call.callee->type == TOKEN_IDENTIFIER) {
		print_token(expr->func_call.callee);
		print_left_paren();

		Expr** args = expr->func_call.args; 
		for (u64 i = 0; i < buf_len(args); ++i) {
			gen_expr(args[i]);

			if (i != (buf_len(args) - 1)) {
				print_string(", ");
			}
		}
		print_right_paren();
	}

	else if (expr->func_call.callee->type == TOKEN_KEYWORD) {
		if (str_intern(expr->func_call.callee->lexeme) ==
			str_intern("set")) {
			gen_set_expr(expr);
		}
	}

	else {
		switch (expr->func_call.callee->type) {
			case TOKEN_PLUS:
			case TOKEN_MINUS:
			case TOKEN_STAR:
			case TOKEN_SLASH: gen_arithmetic_expr(expr); break;

			case TOKEN_EQUAL: gen_comparison_expr(expr); break;

			default: break;	
		}
	}
}

static void gen_set_expr(Expr* expr) {
	print_left_paren();
	gen_variable_expr(expr->func_call.args[0]);
	print_space();
	print_char('=');
	print_space();
	gen_expr(expr->func_call.args[1]);
	print_right_paren();
}

static void gen_arithmetic_expr(Expr* expr) {
	print_left_paren();
	Expr** args = expr->func_call.args;
	for (u64 i = 0; i < buf_len(args); ++i) {
		gen_expr(args[i]);
		
		if (i != (buf_len(args) - 1)) {
			print_token(expr->func_call.callee);
		}
	}
	print_right_paren();
}

static void gen_comparison_expr(Expr* expr) {
	print_left_paren();
	Expr** args = expr->func_call.args;
	for (u64 i = 0; i < buf_len(args); ++i) {
		gen_expr(args[i]);
		
		if (i != (buf_len(args) - 1)) {
			if (expr->func_call.callee->type == TOKEN_EQUAL) {
				print_string("==");
			}
			else {
				print_token(expr->func_call.callee);
			}
		}
	}
	print_right_paren();
}

static void print_data_type(DataType* data_type) {
	print_token(data_type->type);
	for (u8 i = 0; i < data_type->pointer_count; ++i) {
		print_char('*');
	}
}

static void print_token(Token* t) {
	print_string(t->lexeme);
}

static void print_string(char* str) {
	assert(str);
	char* current_char = str;
	while ((*current_char) != '\0') {
		print_char(*current_char);
		current_char++;
	}
}

static void print_tabs_by_indentation(void) {
	for (uint i =  0; i < tab_count; ++i) {
		print_tab();
	}
}

static void print_semicolon(void) {
	print_char(';');
}

static void print_left_paren(void) {
	print_char('(');
}

static void print_right_paren(void) {
	print_char(')');
}

static void print_tab(void) {
	for (uint i = 0; i < TAB_SIZE; ++i) {
		print_space();
	}
}

static void print_space(void) {
	print_char(' ');
}

static void print_newline(void) {
	print_char('\n');
}

static void print_char(char c) {
	buf_push(output_code, c);
}

static void compile_output_code(void) {
	FILE* gcc;
	gcc = popen("gcc -xc -", "w");
	if (!gcc) {
		printf("error!"); /* TODO: nice error message */
		return;
	}
	fputs(output_code, gcc);
	pclose(gcc);
}

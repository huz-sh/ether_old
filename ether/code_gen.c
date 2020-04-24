#include <ether/ether.h>

FILE* popen(const char*, const char*);
int pclose(FILE*);

static Stmt** stmts;
static SourceFile* srcfile;
static char* output_code;
static uint tab_count;

static void code_gen_destroy(void);

static void gen_include_headers(void);
static void gen_include_header(char*);
static void gen_defines(void);
static void gen_define(char*, char*);
static void gen_typedefs(void);
static void gen_typedef(char*, char*);
static void gen_struct_decls(void);
static void gen_struct_decl(Stmt*);
static void gen_structs(void);
static void gen_struct(Stmt*);
static void gen_inline_struct(Stmt*);
static void gen_field(Field*);
static void gen_global_var_decls(void);
static void gen_func_decls(void);
static void gen_func_decl(Stmt*);
static void gen_func_prototype(Stmt*);
static void gen_file(Stmt**);
static void gen_func(Stmt*);
static void gen_stmt(Stmt*);
static void gen_var_decl(Stmt*);
static void gen_if_stmt(Stmt*);
static void gen_if_branch(IfBranch*, IfBranchType);
static void gen_for_stmt(Stmt*);
static void gen_while_stmt(Stmt*);
static void gen_return_stmt(Stmt*);
static void gen_expr_stmt(Stmt*);

static void gen_expr(Expr*);
static void gen_dot_access_expr(Expr*);
static void gen_number_expr(Expr*);
static void gen_char_expr(Expr*);
static void gen_string_expr(Expr*);
static void gen_null_expr(Expr*);
static void gen_variable_expr(Expr*);
static void gen_func_call(Expr*);
static void gen_set_expr(Expr*);
static void gen_deref_expr(Expr*);
static void gen_addr_expr(Expr*);
static void gen_at_expr(Expr*);
static void gen_arithmetic_expr(Expr*);
static void gen_comparison_expr(Expr*);

static void print_data_type(DataType*);
static void print_token(Token*);
static void print_string(char*);
static void print_tabs_by_indentation(void);
static void print_semicolon(void);
static void print_comma(void);
static void print_equal(void);
static void print_left_paren(void);
static void print_right_paren(void);
static void print_left_brace(void);
static void print_right_brace(void);
static void print_tab(void);
static void print_space(void);
static void print_newline(void);
static void print_char(char);

static void compile_output_code(void);

void code_gen_init(Stmt** p_stmts, SourceFile* p_srcfile) {
	stmts = p_stmts;
	srcfile = p_srcfile;
	output_code = null;
	tab_count = 0;
}

void code_gen_run(void) {
	gen_include_headers();
	gen_defines();
	gen_typedefs();
	gen_struct_decls();
	gen_structs();
	gen_global_var_decls();
	gen_func_decls();
	
	gen_file(stmts);
	buf_push(output_code, '\0');
	printf("%s", output_code); /* TODO: remove this */

	compile_output_code();
	
	code_gen_destroy();
}

static void code_gen_destroy(void) {
	buf_free(output_code);
}

static void gen_defines(void) {
	gen_define("null", "(void*)0");

	print_newline();
}

static void gen_define(char* to, char* from) {
	print_string("#define ");
	print_string(to);
	print_space();
	print_string(from);
	print_newline();
}

static void gen_include_headers(void) {
	gen_include_header("stdint.h");

	print_newline();
}

static void gen_include_header(char* name) {
	print_string("#include <");
	print_string(name);
	print_string(">\n");
}

static void gen_typedefs(void) {
	gen_typedef("int8_t", "i8");
	gen_typedef("int16_t", "i16");
	gen_typedef("int32_t", "i32");
	gen_typedef("int64_t", "i64");

	gen_typedef("uint8_t", "u8");
	gen_typedef("uint16_t", "u16");
	gen_typedef("uint32_t", "u32");
	gen_typedef("uint64_t", "u64");

	print_newline();
}

static void gen_typedef(char* from, char* to) {
	print_string("typedef ");
	print_string(from);
	print_space();
	print_string(to);
	print_semicolon();
	print_newline();
}

static void gen_struct_decls(void) {
	for (u64 stmt = 0; stmt < buf_len(stmts); ++stmt) {
		Stmt* current_stmt = stmts[stmt];
		if (current_stmt->type == STMT_STRUCT) {
			gen_struct_decl(current_stmt);
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
	for (u64 stmt = 0; stmt < buf_len(stmts); ++stmt) {
		Stmt* current_stmt = stmts[stmt];
		if (current_stmt->type == STMT_STRUCT) {
			gen_struct(current_stmt);
		}
	}
}

static void gen_struct(Stmt* stmt) {
	print_tabs_by_indentation();
	print_string("typedef struct ");
	print_token(stmt->struct_stmt.identifier);
	print_space();
	print_left_brace();
	print_newline();

	tab_count++;
	Field** fields = stmt->struct_stmt.fields;
	for (u64 i = 0; i < buf_len(fields); ++i) {
		print_tabs_by_indentation();
		gen_field(fields[i]);
	}
	tab_count--;
	print_right_brace();
	print_space();
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
	print_right_brace();
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
	for (u64 stmt = 0; stmt < buf_len(stmts); ++stmt) {
		Stmt* current_stmt = stmts[stmt];
		if (current_stmt->type == STMT_VAR_DECL) {
			gen_var_decl(current_stmt);
		}
	}
	print_newline();
}

static void gen_func_decls(void) {
		for (u64 stmt = 0; stmt < buf_len(stmts); ++stmt) {
			Stmt* current_stmt = stmts[stmt];
			if (current_stmt->type == STMT_FUNC) {
				gen_func_decl(current_stmt);
			}
		}
	print_newline();
}

static void gen_func_decl(Stmt* stmt) {
	gen_func_prototype(stmt);
	print_semicolon();
	print_newline();
}

static void gen_func_prototype(Stmt* stmt) {
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
			print_comma();
			print_space();
		}
	}
	print_right_paren();
}

static void gen_file(Stmt** p_stmts) {
	for (u64 i = 0; i < buf_len(p_stmts); ++i) {
		if (p_stmts[i]->type == STMT_FUNC) {
			if (p_stmts[i]->func.is_function) {
				gen_func(p_stmts[i]);
			}
		}
	}	
}

static void gen_func(Stmt* stmt) {
	gen_func_prototype(stmt);

	print_space();
	print_left_brace();
	print_newline();
	Stmt** body = stmt->func.body;
	tab_count++;
	for (u64 i = 0; i < buf_len(body); ++i) {
		gen_stmt(body[i]);
	}
	tab_count--;
	print_right_brace();
	print_newline();
	print_newline();
}

static void gen_stmt(Stmt* stmt) {
	print_tabs_by_indentation();
	switch (stmt->type) {
		case STMT_VAR_DECL: gen_var_decl(stmt); break;
		case STMT_IF: gen_if_stmt(stmt); break;
		case STMT_FOR: gen_for_stmt(stmt); break;
		case STMT_WHILE: gen_while_stmt(stmt); break;	
		case STMT_RETURN: gen_return_stmt(stmt); break;	
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
		print_space();
		print_equal();
		print_space();
		gen_expr(stmt->var_decl.initializer);
	}
	print_semicolon();
	print_newline();
}

static void gen_if_stmt(Stmt* stmt) {
	gen_if_branch(stmt->if_stmt.if_branch, IF_IF_BRANCH);

	for (u64 i = 0; i < buf_len(stmt->if_stmt.elif_branch); ++i) {
		gen_if_branch(stmt->if_stmt.elif_branch[i], IF_ELIF_BRANCH);
	}

	if (stmt->if_stmt.else_branch) {
		gen_if_branch(stmt->if_stmt.else_branch, IF_ELSE_BRANCH);
	}
}

static void gen_if_branch(IfBranch* branch, IfBranchType type) {
	if (type != IF_IF_BRANCH) {
		print_tabs_by_indentation();
	}

	switch (type) {
		case IF_IF_BRANCH: print_string("if "); break;
		case IF_ELIF_BRANCH: print_string("else if "); break;
		case IF_ELSE_BRANCH: print_string("else "); break;	
	}

	if (type != IF_ELSE_BRANCH) {
		print_left_paren();
		gen_expr(branch->cond);
		print_right_paren();
		print_space();
	}

	print_left_brace();
	print_newline();
	
	tab_count++;
	for (u64 i = 0; i < buf_len(branch->body); ++i) {
		gen_stmt(branch->body[i]);
	}
	tab_count--;
	
	print_tabs_by_indentation();
	print_right_brace();
	print_newline();
}

static void gen_for_stmt(Stmt* stmt) {
	/* TODO: if for loop counter is modifiable, change this */
	print_string("for (int ");
	print_token(stmt->for_stmt.counter->var_decl.identifier);
	print_string(" = 0; ");
	print_token(stmt->for_stmt.counter->var_decl.identifier);
	print_string(" < ");
	gen_expr(stmt->for_stmt.to);
	print_string("; ++");
	print_token(stmt->for_stmt.counter->var_decl.identifier);
	print_string(") {");
	print_newline();

	tab_count++;
	for (u64 i = 0; i < buf_len(stmt->for_stmt.body); ++i) {
		gen_stmt(stmt->for_stmt.body[i]);
	}
	tab_count--;
	
	print_tabs_by_indentation();
	print_right_brace();
	print_newline();
}

static void gen_while_stmt(Stmt* stmt) {
	print_string("while (");
	gen_expr(stmt->while_stmt.cond);
	print_string(") {");
	print_newline();

	tab_count++;
	for (u64 i = 0; i < buf_len(stmt->while_stmt.body); ++i) {
		gen_stmt(stmt->while_stmt.body[i]);
	}
	tab_count--;
	
	print_tabs_by_indentation();
	print_right_brace();
	print_newline();
	
}

static void gen_return_stmt(Stmt* stmt) {
	print_string("return ");
	if (stmt->return_stmt.expr) {
		print_left_paren();
		gen_expr(stmt->return_stmt.expr);
		print_right_paren();
	}
	print_semicolon();
	print_newline();
}

static void gen_expr_stmt(Stmt* stmt) {
	gen_expr(stmt->expr);
	print_semicolon();
	print_newline();
}

static void gen_expr(Expr* expr) {
	switch (expr->type) {
		case EXPR_DOT_ACCESS: gen_dot_access_expr(expr); break;
		case EXPR_NUMBER: gen_number_expr(expr); break;
		case EXPR_CHAR: gen_char_expr(expr); break;	
		case EXPR_STRING: gen_string_expr(expr); break;
		case EXPR_NULL: gen_null_expr(expr); break;
		case EXPR_VARIABLE: gen_variable_expr(expr); break;
		case EXPR_FUNC_CALL: gen_func_call(expr); break;	
	}
}

static void gen_dot_access_expr(Expr* expr) {
	print_left_paren();
	print_left_paren();
	gen_expr(expr->dot.left);
	print_right_paren();
	print_string(expr->dot.is_left_pointer ? "->" : ".");
	print_token(expr->dot.right);
	print_right_paren();
}

static void gen_number_expr(Expr* expr) {
	print_token(expr->number);
}

static void gen_char_expr(Expr* expr) {
	print_char('\'');
	print_token(expr->chr);
	print_char('\'');
}

static void gen_string_expr(Expr* expr) {
	print_char('\"');
	print_token(expr->string);
	print_char('\"');
}

static void gen_null_expr(Expr* expr) {
	print_string("null");
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
				print_comma();
				print_space();
			}
		}
		print_right_paren();
	}

	else if (expr->func_call.callee->type == TOKEN_KEYWORD) {
		if (str_intern(expr->func_call.callee->lexeme) ==
			str_intern("set")) {
			gen_set_expr(expr);
		}
		else if (str_intern(expr->func_call.callee->lexeme) ==
			str_intern("deref")) {
			gen_deref_expr(expr);
		}
		else if (str_intern(expr->func_call.callee->lexeme) ==
			str_intern("addr")) {
			gen_addr_expr(expr);
		}
		else if (str_intern(expr->func_call.callee->lexeme) ==
			str_intern("at")) {
			gen_at_expr(expr);
		}
	}

	else {
		switch (expr->func_call.callee->type) {
			case TOKEN_PLUS:
			case TOKEN_MINUS:
			case TOKEN_STAR:
			case TOKEN_SLASH: 
			case TOKEN_PERCENT:
				gen_arithmetic_expr(expr); return;

			case TOKEN_EQUAL: 
			case TOKEN_LESS:
			case TOKEN_LESS_EQUAL:
			case TOKEN_GREATER:
			case TOKEN_GREATER_EQUAL: 
				gen_comparison_expr(expr); return;

			default: return;	
		}
	}
}

static void gen_set_expr(Expr* expr) {
	print_left_paren();
	gen_expr(expr->func_call.args[0]);
	print_space();
	print_equal();
	print_space();
	gen_expr(expr->func_call.args[1]);
	print_right_paren();
}

static void gen_deref_expr(Expr* expr) {
	print_left_paren();
	print_char('*');
	gen_expr(expr->func_call.args[0]);
	print_right_paren();
}

static void gen_addr_expr(Expr* expr) {
	print_left_paren();
	print_char('&');
	gen_expr(expr->func_call.args[0]);
	print_right_paren();
}

static void gen_at_expr(Expr* expr) {
	print_left_paren();
	print_left_paren();
	gen_expr(expr->func_call.args[0]);
	print_right_paren();
	print_char('[');
	gen_expr(expr->func_call.args[1]);
	print_char(']');
	print_right_paren();
}

static void gen_arithmetic_expr(Expr* expr) {
	print_left_paren();
	Expr** args = expr->func_call.args;
	for (u64 i = 0; i < buf_len(args); ++i) {
		gen_expr(args[i]);
		
		if (i != (buf_len(args) - 1)) {
			print_space();
			print_token(expr->func_call.callee);
			print_space();
		}
	}
	print_right_paren();
}

static void gen_comparison_expr(Expr* expr) {
	print_left_paren();
	Expr** args = expr->func_call.args;
	gen_expr(args[0]);
	print_space();
	if (expr->func_call.callee->type == TOKEN_EQUAL) {
		print_equal();
		print_equal();
	}
	else {
		print_token(expr->func_call.callee);
	}
	print_space();
	gen_expr(args[1]);
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

static void print_comma(void) {
	print_char(',');
}

static void print_equal(void) {
	print_char('=');
}

static void print_left_paren(void) {
	print_char('(');
}

static void print_right_paren(void) {
	print_char(')');
}

static void print_left_brace(void) {
	print_char('{');
}

static void print_right_brace(void) {
	print_char('}');
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
	char* command_first_part = "gcc -g -w -fno-stack-protector -nostdlib -c -o ";
	u64 command_first_part_len = strlen(command_first_part);
	
	char* command_last_part = " -xc -";
	u64 command_last_part_len = strlen(command_last_part);

	u64 filename_len = (strlen(srcfile->fpath) - 2);
	u64 command_len = command_first_part_len +
					  command_last_part_len +
					  filename_len;
	char* command = malloc(command_len + 1);
	strncpy(command, command_first_part, command_first_part_len);
	strncpy(command + command_first_part_len,
			srcfile->fpath, filename_len - 1);
	command[command_first_part_len + filename_len - 1] = 'o';
	strncpy(command + command_first_part_len + filename_len,
			command_last_part, command_last_part_len);
	command[command_len] = '\0';
	printf("command: ");
	printf(command);
	printf("\n");
	
	gcc = popen(command, "w");
	if (!gcc) {
		printf("error!"); /* TODO: nice error message */
		return;
	}
	fputs(output_code, gcc);
	pclose(gcc);
}

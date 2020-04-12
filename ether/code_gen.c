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
static void gen_function_prototypes(void);
static void gen_file(Stmt**);
static void gen_func(Stmt*);
static void gen_stmt(Stmt*);
static void gen_var_decl(Stmt*);
static void gen_if_stmt(Stmt*);
static void gen_expr_stmt(Stmt*);

static void print_data_type(DataType*);
static void print_token(Token*);
static void print_string(char*);
static void print_tabs_by_indentation(void);
static void print_semicolon(void);
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
	gen_function_prototypes();
	
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

static void gen_function_prototypes(void) {
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
	/* TODO: print initializer */
	print_semicolon();
	print_newline();
}

static void gen_if_stmt(Stmt* stmt) {
}

static void gen_expr_stmt(Stmt* stmt) {
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

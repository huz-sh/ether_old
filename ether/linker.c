#include <ether/ether.h>

static Stmt*** stmts_all;
static Stmt** defined_structs;
static Stmt** defined_functions;
static bool main_func_found;
static Scope* global_scope;
static Scope* current_scope;
static Scope** all_scopes;

static bool error_occured;
static uint error_count;

static void linker_destroy(void);

static void link_file(Stmt**);
static void add_decl_stmt(Stmt*);

static void check_file(Stmt**);
static void check_stmt(Stmt* stmt);
static void check_struct(Stmt*);
static void check_func(Stmt*);
static void check_var_decl(Stmt*);
static void check_data_type(DataType* data_type);

static bool is_variable_declared(Stmt*);
static bool is_token_identical(Token*, Token*);
static Scope* make_scope(Scope*);
static void add_variable_to_scope(Stmt*);

static void error(Token*, const char*, ...);
static void note(Token*, const char*, ...);
static void error_without_info(const char*, ...);

void linker_init(Stmt*** stmts_buf) {
	stmts_all = stmts_buf;
	main_func_found = false;
	all_scopes = null;
	global_scope = make_scope(null);
	current_scope = global_scope;
}

error_code linker_run(void) {
	/* we assume that the length of srcfiles is equal to the 
	 * length of stmts_all */
	for (u64 file = 0; file < buf_len(stmts_all); ++file) {
		link_file(stmts_all[file]);		
	}
	if (!main_func_found) {
		error_without_info("'main' symbol not found; did you forget to define 'main'?");
		linker_destroy();
		return error_occured;
	}

	for (u64 file = 0; file < buf_len(stmts_all); ++file) {
		check_file(stmts_all[file]);
	}
	
	linker_destroy();
	return error_occured;
}

static void linker_destroy(void) {
	buf_free(defined_structs);
	buf_free(defined_functions);
	
	for (u64 i = 0; i < buf_len(all_scopes); ++i) {
		buf_free(all_scopes[i]->variables);
		free(all_scopes[i]);
	}
	buf_free(all_scopes);
	
	global_scope = null;
	current_scope = null;
}

static void link_file(Stmt** stmts) {
	for (u64 i = 0; i < buf_len(stmts); ++i) {
		add_decl_stmt(stmts[i]);
	}
}

static void add_decl_stmt(Stmt* stmt) {
	if (stmt->type == STMT_STRUCT) {
		for (u64 i = 0; i < buf_len(defined_structs); ++i) {
			if (is_token_identical(stmt->struct_stmt.identifier,
								   defined_structs[i]->struct_stmt.identifier)) {
				error(stmt->struct_stmt.identifier,
					  "redefinition of struct '%s':",
					  stmt->struct_stmt.identifier->lexeme);
				note(defined_structs[i]->struct_stmt.identifier,
					 "struct '%s' previously defined here: ",
					 defined_structs[i]->struct_stmt.identifier->lexeme);
				return;
			}
		}
		buf_push(defined_structs, stmt);
	}

	else if (stmt->type == STMT_FUNC) {
		for (u64 i = 0; i < buf_len(defined_functions); ++i) {
			if (is_token_identical(stmt->func.identifier,
								   defined_functions[i]->func.identifier)) {
				error(stmt->func.identifier,
					  "redefinition of function '%s':",
					  stmt->func.identifier->lexeme);
				note(defined_functions[i]->func.identifier,
					 "function '%s' previously defined here: ",
					 defined_functions[i]->func.identifier->lexeme);
				return;
			}
		}
		if (str_intern(stmt->func.identifier->lexeme) ==
			str_intern("main")) {
			main_func_found = true;
		}
		buf_push(defined_functions, stmt);
	}

	else if (stmt->type == STMT_VAR_DECL) {
		if (is_variable_declared(stmt)) {
			return;			
		}
		else {
			add_variable_to_scope(stmt);
		}
	}
}

static void check_file(Stmt** stmts) {
	for (u64 i = 0; i < buf_len(stmts); ++i) {
		check_stmt(stmts[i]);
	}
}

static void check_stmt(Stmt* stmt) {
	switch (stmt->type) {
		case STMT_STRUCT:   check_struct(stmt); break;
		case STMT_FUNC:     check_func(stmt); break;
		case STMT_VAR_DECL: check_var_decl(stmt); break;	
	}
}

static void check_struct(Stmt* stmt) {
	Field** fields = stmt->struct_stmt.fields;
	for (u64 i = 0; i < buf_len(fields); ++i) {
		check_data_type(fields[i]->type);
	}
}

static void check_func(Stmt* stmt) {
	check_data_type(stmt->func.type);
	for (u64 i = 0; i < buf_len(stmt->func.body); ++i) {
		check_stmt(stmt->func.body[i]);
	}
}

static void check_var_decl(Stmt* stmt) {
	check_data_type(stmt->var_decl.type);
	/* TODO: fill this for initializer */	
}

static void check_data_type(DataType* data_type) {
	assert(data_type);
	if (data_type->type->type == TOKEN_IDENTIFIER) {
		bool data_type_valid = false;
		for (u64 i = 0; i < buf_len(defined_structs); ++i) {
			if (is_token_identical(data_type->type,
								   defined_structs[i]->struct_stmt.identifier)) {
				data_type_valid = true;
			}
		}
	
		if (!data_type_valid) {
			error(data_type->type,
				  "undefined type name '%s'; did you forget to define type '%s'",
				  data_type->type->lexeme,
				  data_type->type->lexeme);
		}
	}
}

static bool is_variable_declared(Stmt* var) {
	for (u64 i = 0; i < buf_len(current_scope->variables); ++i) {
		if (is_token_identical(var->var_decl.identifier,
							   current_scope->variables[i]->var_decl.identifier)) {
			error(var->var_decl.identifier,
				  "redeclaration of variable '%s':",
				  var->var_decl.identifier->lexeme);
			note(current_scope->variables[i]->var_decl.identifier,
				 "variable '%s' previously declared here: ",
				 current_scope->variables[i]->var_decl.identifier->lexeme);
			return false;
		}
	}
	return true;
}

static bool is_token_identical(Token* a, Token* b) {
	assert(a);
	assert(b);

	if (str_intern(a->lexeme) == str_intern(b->lexeme)) {
		return true;
	}
	return false;
}

static Scope* make_scope(Scope* parent_scope) {
	Scope* scope = (Scope*)malloc(sizeof(Scope));
	scope->variables = null;
	scope->parent_scope = parent_scope;
	buf_push(all_scopes, scope);
	return scope;
}

static void add_variable_to_scope(Stmt* var) {
	buf_push(current_scope->variables, var);
}

static void error(Token* token, const char* fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	printf("%s:%ld:%d: error: ",
		   token->srcfile->fpath, token->line, token->column);
	vprintf(fmt, ap);
	va_end(ap);
	printf("\n");

	print_file_line_with_info(token->srcfile, token->line);
	print_marker_arrow_with_info_ln(token->srcfile, token->line, token->column);

	error_occured = ETHER_ERROR;
	++error_count;
}

static void note(Token* token, const char* fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	printf("%s:%ld:%d: note: ",
		   token->srcfile->fpath, token->line, token->column);
	vprintf(fmt, ap);
	va_end(ap);
	printf("\n");

	print_file_line_with_info(token->srcfile, token->line);
	print_marker_arrow_with_info_ln(token->srcfile, token->line, token->column);	
}

static void error_without_info(const char* fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	printf("error: ");
	vprintf(fmt, ap);
	va_end(ap);
	printf("\n");

	error_occured = ETHER_ERROR;
	++error_count;	
}

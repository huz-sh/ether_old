#include <ether/ether.h>

static SourceFile* srcfiles;
static Stmt*** stmts_all;

static SourceFile* current_file;
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

static bool is_token_identical(Token*, Token*);
static Scope* make_scope(Scope*);

static void error(Token*, const char*, ...);
static void note(Token*, const char*, ...);
static void error_without_info(const char*, ...);

void linker_init(SourceFile* files, Stmt*** stmts_buf) {
	srcfiles = files;
	stmts_all = stmts_buf;
	assert(buf_len(srcfiles) == buf_len(stmts_all));
	main_func_found = false;
	all_scopes = null;
	global_scope = make_scope(null);
	current_scope = global_scope;
}

error_code linker_run(void) {
	/* we assume that the length of srcfiles is equal to the 
	 * length of stmts_all */
	for (u64 file = 0; file < buf_len(srcfiles); ++file) {
		current_file = &(srcfiles[file]);
		link_file(stmts_all[file]);		
	}
	if (!main_func_found) {
		error_without_info("'main' symbol not found; did you forget to define 'main'?");
		linker_destroy();
		return error_occured;
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
		for (u64 i = 0; i < buf_len(global_scope->variables); ++i) {
			if (is_token_identical(stmt->var_decl.identifier,
								   global_scope->variables[i]->var_decl.identifier)) {
				error(stmt->var_decl.identifier,
					  "redeclaration of variable '%s':",
					  stmt->var_decl.identifier->lexeme);
				note(global_scope->variables[i]->var_decl.identifier,
					 "variable '%s' previously declared here: ",
					 global_scope->variables[i]->var_decl.identifier->lexeme);
				return;
			}
		}
		buf_push(global_scope->variables, stmt);
	}
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

static void error(Token* token, const char* fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	printf("%s:%ld:%d: error: ",
		   current_file->fpath, token->line, token->column);
	vprintf(fmt, ap);
	va_end(ap);
	printf("\n");

	print_file_line_with_info(*current_file, token->line);
	print_marker_arrow_with_info_ln(*current_file, token->line, token->column);

	error_occured = ETHER_ERROR;
	++error_count;
}

static void note(Token* token, const char* fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	printf("%s:%ld:%d: note: ",
		   current_file->fpath, token->line, token->column);
	vprintf(fmt, ap);
	va_end(ap);
	printf("\n");

	print_file_line_with_info(*current_file, token->line);
	print_marker_arrow_with_info_ln(*current_file, token->line, token->column);	
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

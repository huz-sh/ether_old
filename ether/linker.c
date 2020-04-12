#include <ether/ether.h>
#include <ether/linker_resolve_common.h>

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
static void check_stmt(Stmt*);
static void check_struct(Stmt*);
static void check_func(Stmt*);
static void check_global_var_decl(Stmt*);
static void check_var_decl(Stmt*);
static void check_if_stmt(Stmt*);
static void check_if_branch(IfBranch*, IfBranchType);
static void check_expr_stmt(Stmt*);
static void check_expr(Expr*);
static void check_func_call(Expr*);
static void check_set_expr(Expr*);
static void check_comparision_expr(Expr*);
static void check_variable_expr(Expr*);

static void check_data_type(DataType*);
static void check_if_variable_is_in_scope(Expr*);
static bool is_variable_declared(Stmt*);

static Scope* make_scope(Scope*);
static void add_variable_to_scope(Stmt*);

static void error_without_info(const char*, ...);

#define CHANGE_SCOPE(x) \
	Scope* x = make_scope(current_scope); \
	current_scope = x;

#define REVERT_SCOPE(x) \
	current_scope = x->parent_scope;

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
		if (!is_variable_declared(stmt)) {
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
		case STMT_STRUCT: check_struct(stmt); break;
		case STMT_FUNC: check_func(stmt); break;
		case STMT_VAR_DECL: {
			if (stmt->var_decl.is_global_var) {
				check_global_var_decl(stmt);
			}
			else {
				check_var_decl(stmt);
			}
		} break;
		case STMT_IF: check_if_stmt(stmt); break;
		case STMT_EXPR: check_expr_stmt(stmt); break;
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
	CHANGE_SCOPE(scope);

	for (u64 param = 0; param < buf_len(stmt->func.params); ++param) {
		if (!is_variable_declared(stmt->func.params[param])) {
			add_variable_to_scope(stmt->func.params[param]);
		}
	}
	
	for (u64 i = 0; i < buf_len(stmt->func.body); ++i) {
		check_stmt(stmt->func.body[i]);
	}
	REVERT_SCOPE(scope);
}

static void check_global_var_decl(Stmt* stmt) {
	check_data_type(stmt->var_decl.type);
	if (stmt->var_decl.initializer) {
		/* TODO: add current variable to scope to reference it like this:
		 * [let int:var [+ var 1]] */
		check_expr(stmt->var_decl.initializer);
	}
}

static void check_var_decl(Stmt* stmt) {
	check_data_type(stmt->var_decl.type);
	if (stmt->var_decl.initializer) {
		/* TODO: add current variable to scope to reference it like this:
		 * [let int:var [+ var 1]] */
		check_expr(stmt->var_decl.initializer);
	}

	/* always remember to add this at the end of check_var_decl,
	 * not in the middle */
	if (!is_variable_declared(stmt)) {
		add_variable_to_scope(stmt);
	}
}

static void check_if_stmt(Stmt* stmt) {
	check_if_branch(stmt->if_stmt.if_branch, IF_IF_BRANCH);

	for (u64 i = 0; i < buf_len(stmt->if_stmt.elif_branch); ++i) {
		check_if_branch(stmt->if_stmt.elif_branch[i], IF_ELIF_BRANCH);
	}

	if (stmt->if_stmt.else_branch) {
		check_if_branch(stmt->if_stmt.else_branch, IF_ELSE_BRANCH);
	}
}

static void check_if_branch(IfBranch* branch, IfBranchType type) {
	if (type != IF_ELSE_BRANCH) {
		check_expr(branch->cond);
	}

	CHANGE_SCOPE(scope);
	for (u64 i = 0; i < buf_len(branch->body); ++i) {
		check_stmt(branch->body[i]);
	}
	REVERT_SCOPE(scope);		
}

static void check_expr_stmt(Stmt* stmt) {
	check_expr(stmt->expr);
}

static void check_expr(Expr* expr) {
	switch (expr->type) {
		case EXPR_FUNC_CALL: check_func_call(expr); break;
		case EXPR_VARIABLE: check_variable_expr(expr); break;
		case EXPR_NUMBER: break;	
	}
}

static void check_func_call(Expr* expr) {
	if (expr->func_call.callee->type == TOKEN_IDENTIFIER) {
		for (u64 i = 0; i < buf_len(defined_functions); ++i) {
			if (is_token_identical(expr->func_call.callee,
								   defined_functions[i]->func.identifier)) {
				const u64 caller_args_len = buf_len(expr->func_call.args);
				const u64 callee_params_len = buf_len(defined_functions[i]->func.params);
				Token* error_token = null;

				if (caller_args_len > callee_params_len) {
					error_token = expr->func_call.args[callee_params_len]->head;
				}
				else if (caller_args_len < callee_params_len) {
					error_token = expr->head;
				}
				
				if (error_token != null) {
					error(error_token,
						  "conflicting argument-length in function call; "
						  "expected %ld argument(s), but got %ld argument(s);",
						  callee_params_len, caller_args_len);
					note(defined_functions[i]->func.identifier,
						 "callee '%s' defined here:",
						 defined_functions[i]->func.identifier->lexeme);
					return;
				}
				
				for (u64 arg = 0; arg < caller_args_len; ++arg) {
					check_expr(expr->func_call.args[arg]);
				}
				expr->func_call.function_called = defined_functions[i];
				return;
			}
		}

		error(expr->func_call.callee,
			  "undefined function '%s'; did you forget to define '%s'?",
			  expr->func_call.callee->lexeme,
			  expr->func_call.callee->lexeme);
		return;
	} 
	
	else if (expr->func_call.callee->type == TOKEN_KEYWORD) {
		if (str_intern(expr->func_call.callee->lexeme) ==
			str_intern("set")) {
			check_set_expr(expr);
		}
	}

	else {
		switch (expr->func_call.callee->type) {
			case TOKEN_EQUAL: check_comparision_expr(expr); return;
		}
		
		for (u64 i = 0; i < buf_len(expr->func_call.args); ++i) {
			check_expr(expr->func_call.args[i]);
		}
	}
}

static void check_set_expr(Expr* expr) {
	u64 args_len = buf_len(expr->func_call.args);
	Token* error_token = null;
								  
	if (args_len > 2) {
		error_token = expr->func_call.args[2]->head;
	}
	else if (args_len < 2) {
		error_token = expr->head;
	}
			
	if (error_token != null) {
		/* TODO: change 'set' to a macro */
		error(error_token,
			  "built-in function 'set' only accepts 2 arguments, "
			  "but got %ld argument(s);", args_len);
		return;
	}

	check_if_variable_is_in_scope(expr->func_call.args[0]);
	check_expr(expr->func_call.args[1]);
}

static void check_comparision_expr(Expr* expr) {
	u64 args_len = buf_len(expr->func_call.args);
	Token* error_token = null;
								  
	if (args_len > 2) {
		error_token = expr->func_call.args[2]->head;
	}
	else if (args_len < 2) {
		error_token = expr->head;
	}
			
	if (error_token != null) {
		error(error_token,
			  "'=' operator only accepts 2 arguments, "
			  "but got %ld argument(s);", args_len);
		return;
	}

	check_expr(expr->func_call.args[0]);	
	check_expr(expr->func_call.args[1]);	
}

static void check_variable_expr(Expr* expr) {
	check_if_variable_is_in_scope(expr);
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
	Scope* scope = current_scope;
	while (scope != null) {
		for (u64 i = 0; i < buf_len(scope->variables); ++i) {
			if (is_token_identical(var->var_decl.identifier,
								   scope->variables[i]->var_decl.identifier)) {
				error(var->var_decl.identifier,
					  "redeclaration of variable '%s':",
					  var->var_decl.identifier->lexeme);
				note(scope->variables[i]->var_decl.identifier,
					 "variable '%s' previously declared here: ",
					 scope->variables[i]->var_decl.identifier->lexeme);
				return true;
			}
		}
		scope = scope->parent_scope;
	}
	return false;
}

static void check_if_variable_is_in_scope(Expr* expr) {
	Scope* scope = current_scope;
	while (scope != null) {
		for (u64 i = 0; i < buf_len(scope->variables); ++i) {
			if (is_token_identical(expr->variable.identifier,
								   scope->variables[i]->var_decl.identifier)) {
				expr->variable.variable_decl_referenced = scope->variables[i];
				return;
			}
		}
		scope = scope->parent_scope;
	}

	error(expr->variable.identifier,
		  "undeclared variable '%s'; did you forget to declare '%s'?",
		  expr->variable.identifier->lexeme, expr->variable.identifier->lexeme);
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

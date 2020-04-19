#include <ether/ether.h>
#include <ether/linker_resolve_code_gen_common.h>

static Stmt** stmts;
static Stmt** defined_structs;
static Stmt** defined_functions;
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
static void check_func_decl(Stmt*);
static void check_global_var_decl(Stmt*);
static void check_var_decl(Stmt*);
static void check_if_stmt(Stmt*);
static void check_if_branch(IfBranch*, IfBranchType);
static void check_return_stmt(Stmt*);
static void check_expr_stmt(Stmt*);

static void check_expr(Expr*);
static void check_dot_access_expr(Expr*);
static void check_func_call(Expr*);
static void check_set_expr(Expr*);
static void check_deref_expr(Expr*);
static void check_addr_expr(Expr*);
static void check_at_expr(Expr*);
static void check_arithmetic_expr(Expr*);
static void check_comparision_expr(Expr*);
static void check_variable_expr(Expr*);

static void check_data_type(DataType*);
static Stmt* check_data_type_return_struct_if_identifier(DataType*_type);
static void check_if_variable_is_in_scope(Expr*);
static bool is_variable_declared(Stmt*);
static bool func_decls_match(Stmt*, Stmt*);

static Scope* make_scope(Scope*);
static void add_variable_to_scope(Stmt*);

#define CHANGE_SCOPE(x) \
	Scope* x = make_scope(current_scope); \
	current_scope = x;

#define REVERT_SCOPE(x) \
	current_scope = x->parent_scope;

void linker_init(Stmt** p_stmts) {
	stmts = p_stmts;
	all_scopes = null;
	global_scope = make_scope(null);
	current_scope = global_scope;
}

Stmt** linker_run(error_code* err_code) {
	link_file(stmts);		
	check_file(stmts);
	
	linker_destroy();
	if (err_code) *err_code = error_occured;
	return defined_structs;
}

static void linker_destroy(void) {
	buf_free(defined_functions);
	
	for (u64 i = 0; i < buf_len(all_scopes); ++i) {
		buf_free(all_scopes[i]->variables);
		free(all_scopes[i]);
	}
	buf_free(all_scopes);
	
	global_scope = null;
	current_scope = null;
}

static void link_file(Stmt** p_stmts) {
	for (u64 i = 0; i < buf_len(p_stmts); ++i) {
		add_decl_stmt(p_stmts[i]);
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
				if (stmt->func.is_function) {
					error(stmt->func.identifier,
						  "redefinition of function '%s':",
						  stmt->func.identifier->lexeme);
					note(defined_functions[i]->func.identifier,
						 "function '%s' previously defined here: ",
						 defined_functions[i]->func.identifier->lexeme);
					return;
				}
				else {
					if (!func_decls_match(defined_functions[i], stmt)) {
						return;
					}
				}
			}
		}
		buf_push(defined_functions, stmt);
	}

	else if (stmt->type == STMT_VAR_DECL) {
		if (!is_variable_declared(stmt)) {
			add_variable_to_scope(stmt);
		}
	}
}

static void check_file(Stmt** p_stmts) {
	for (u64 i = 0; i < buf_len(p_stmts); ++i) {
		check_stmt(p_stmts[i]);
	}
}

static void check_stmt(Stmt* stmt) {
	switch (stmt->type) {
		case STMT_STRUCT: check_struct(stmt); break;
		case STMT_FUNC: {
			if (stmt->func.is_function) {
				check_func(stmt);
			}
			else {
				check_func_decl(stmt);
			}
		} break;
			
		case STMT_VAR_DECL: {
			if (stmt->var_decl.is_global_var) {
				check_global_var_decl(stmt);
			}
			else {
				check_var_decl(stmt);
			}
		} break;
		case STMT_IF: check_if_stmt(stmt); break;
		case STMT_RETURN: check_return_stmt(stmt); break;	
		case STMT_EXPR: check_expr_stmt(stmt); break;
	}
}

static void check_struct(Stmt* stmt) {
	Field** fields = stmt->struct_stmt.fields;
	for (u64 i = 0; i < buf_len(fields); ++i) {
		if (fields[i]->type->type->type == TOKEN_IDENTIFIER) {
			fields[i]->struct_referenced =
				check_data_type_return_struct_if_identifier(fields[i]->type);
		}
		else {
			check_data_type(fields[i]->type);
		}
	}
}

static void check_func(Stmt* stmt) {
	if (stmt->func.type->type->type == TOKEN_KEYWORD) {
		if (str_intern(stmt->func.type->type->lexeme) ==
			str_intern("void") &&
			stmt->func.type->pointer_count == 0) {
		}
	}
	else {
		check_data_type(stmt->func.type);
	}
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

static void check_func_decl(Stmt* stmt) {
	if (stmt->func.type->type->type == TOKEN_KEYWORD) {
		if (str_intern(stmt->func.type->type->lexeme) ==
			str_intern("void") &&
			stmt->func.type->pointer_count == 0) {
		}
	}
	else {
		check_data_type(stmt->func.type);
	}
	CHANGE_SCOPE(scope);

	for (u64 param = 0; param < buf_len(stmt->func.params); ++param) {
		if (!is_variable_declared(stmt->func.params[param])) {
			add_variable_to_scope(stmt->func.params[param]);
		}
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

static void check_return_stmt(Stmt* stmt) {
	if (stmt->return_stmt.expr) {
		check_expr(stmt->return_stmt.expr);
	}
}

static void check_expr_stmt(Stmt* stmt) {
	check_expr(stmt->expr);
}

static void check_expr(Expr* expr) {
	switch (expr->type) {
		case EXPR_DOT_ACCESS: check_dot_access_expr(expr); break;
		case EXPR_FUNC_CALL: check_func_call(expr); break;
		case EXPR_VARIABLE: check_variable_expr(expr); break;
		case EXPR_NUMBER:
		case EXPR_CHAR:	
		case EXPR_STRING: break;
	}
}

static void check_dot_access_expr(Expr* expr) {
	check_expr(expr->dot.left);	
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
			  "implicit declaration of function '%s'; did you forget to define '%s'?"
			  " (use a 'decl' statement to suppress this error);",
			  expr->func_call.callee->lexeme,
			  expr->func_call.callee->lexeme);
		return;
	} 
	
	else if (expr->func_call.callee->type == TOKEN_KEYWORD) {
		if (str_intern(expr->func_call.callee->lexeme) ==
			str_intern("set")) {
			check_set_expr(expr);
		}
		else if (str_intern(expr->func_call.callee->lexeme) ==
				 str_intern("deref")) {
			check_deref_expr(expr);
		}
		else if (str_intern(expr->func_call.callee->lexeme) ==
				 str_intern("addr")) {
			check_addr_expr(expr);
		}
		else if (str_intern(expr->func_call.callee->lexeme) ==
				 str_intern("at")) {
			check_at_expr(expr);
		}
	}

	else {
		switch (expr->func_call.callee->type) {
			case TOKEN_PLUS:
			case TOKEN_MINUS:
			case TOKEN_STAR:
			case TOKEN_SLASH: check_arithmetic_expr(expr); break;

			case TOKEN_EQUAL: check_comparision_expr(expr); return;
			default: break;	
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
			  "built-in operator 'set' needs 2 arguments to operate, "
			  "but got %ld argument(s);", args_len);
		return;
	}

	check_expr(expr->func_call.args[0]);
	check_expr(expr->func_call.args[1]);
}

static void check_deref_expr(Expr* expr) {
	u64 args_len = buf_len(expr->func_call.args);

	if (args_len > 1) {
		/* TODO: change 'at' to a macro */
		error(expr->func_call.args[1]->head,
			  "built-in operator 'deref' needs 1 argument to operate, "
			  "but got %ld argument(s);", args_len);
		return;
	}

	check_expr(expr->func_call.args[0]);
}

static void check_addr_expr(Expr* expr) {
	u64 args_len = buf_len(expr->func_call.args);

	if (args_len > 1) {
		/* TODO: change 'at' to a macro */
		error(expr->func_call.args[1]->head,
			  "built-in operator 'addr' needs 1 argument to operate, "
			  "but got %ld argument(s);", args_len);
		return;
	}

	check_expr(expr->func_call.args[0]);
}

static void check_at_expr(Expr* expr) {
	u64 args_len = buf_len(expr->func_call.args);
	Token* error_token = null;
								  
	if (args_len > 2) {
		error_token = expr->func_call.args[2]->head;
	}
	else if (args_len < 2) {
		error_token = expr->head;
	}
			
	if (error_token != null) {
		/* TODO: change 'at' to a macro */
		error(error_token,
			  "built-in operator 'at' needs 2 arguments to operate, "
			  "but got %ld argument(s);", args_len);
		return;
	}

	check_expr(expr->func_call.args[0]);
	check_expr(expr->func_call.args[1]);
}

static void check_arithmetic_expr(Expr* expr) {
	const u64 args_len = buf_len(expr->func_call.args);
	Token* error_token = null;

	if (args_len < 2) {
		error_token = expr->head;
	}

	if (error_token != null) {
		error(error_token,
			  "'%s' operator needs 2 (or more) arguments to operate, "
			  "but got %ld argument(s);",
			  expr->func_call.callee->lexeme,
			  args_len);
		return;
	}

	for (u64 i = 0; i < args_len; ++i) {
		check_expr(expr->func_call.args[i]);		
	}
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
			  "'=' operator need 2 arguments to operate, "
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
	check_data_type_return_struct_if_identifier(data_type);
}

static Stmt* check_data_type_return_struct_if_identifier(DataType* data_type) {
	assert(data_type);
	if (data_type->type->type == TOKEN_IDENTIFIER) {
		Stmt* struct_stmt = null;
		for (u64 i = 0; i < buf_len(defined_structs); ++i) {
			if (is_token_identical(data_type->type,
								   defined_structs[i]->struct_stmt.identifier)) {
				struct_stmt = defined_structs[i];
			}
		}
	
		if (!struct_stmt) {
			error(data_type->type,
				  "undefined type name '%s'; did you forget to define type '%s'",
				  data_type->type->lexeme,
				  data_type->type->lexeme);
			return null;
		}
		return struct_stmt;
	}
	else if (data_type->type->type == TOKEN_KEYWORD) {
		if (str_intern(data_type->type->lexeme) ==
			str_intern("void") &&
			data_type->pointer_count == 0) {
			error(data_type->type,
				  "data type declared 'void' here: ");
			return null;
		}
	}
	return null;
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

static bool func_decls_match(Stmt* a, Stmt* b) {
	bool do_match = true;
	if (str_intern(a->func.type->type->lexeme) !=
		str_intern(b->func.type->type->lexeme)) {
		error(b->func.type->type,
			  "conflicting return types for function '%s';",
			  a->func.identifier->lexeme);
		note(a->func.type->type,
			 "previously declared/defined here: ");
		do_match = false;
	}

	bool do_params_count_match = true;
	if (buf_len(a->func.params) != buf_len(b->func.params)) {
		error(b->func.identifier,
			  "conflicting parameter count for function '%s';",
			  a->func.identifier->lexeme);
		note(a->func.type->type,
			 "previously declared/defined here: ");
		do_match = false;
		do_params_count_match = false;
	}

	if (do_params_count_match) {
		for (u64 i = 0; i < buf_len(a->func.params); ++i) {
			if ((str_intern(a->func.params[i]->var_decl.type->type->lexeme) !=
				 str_intern(b->func.params[i]->var_decl.type->type->lexeme)) ||
				a->func.params[i]->var_decl.type->pointer_count !=
				b->func.params[i]->var_decl.type->pointer_count) {
				error(b->func.params[i]->var_decl.type->type,
					  "conflicting parameter types for function '%s'",
					  a->func.identifier->lexeme);
				note(a->func.params[i]->var_decl.type->type,
					 "previously declared/defined here: ");
				do_match = false;
			}

			if (str_intern(a->func.params[i]->var_decl.identifier->lexeme) !=
				str_intern(a->func.params[i]->var_decl.identifier->lexeme)) {
				error(b->func.params[i]->var_decl.identifier,
					  "conflicting parameter names for function '%s'",
					  a->func.identifier->lexeme);
				note(a->func.params[i]->var_decl.identifier,
					 "previously declared/defined here: ");
				do_match = false;
			}
		}
	}

	return do_match;
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

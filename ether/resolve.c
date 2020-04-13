#include <ether/ether.h>
#include <ether/linker_resolve_code_gen_common.h>

static Stmt*** stmts_all;
static char** data_type_strings;
static bool error_occured;
static bool persistent_error_occured;
static uint error_count;

static DataType* int_data_type;
static DataType* string_data_type;
static DataType* bool_data_type;

static void resolve_destroy(void);

static void resolve_file(Stmt**);
static void resolve_stmt(Stmt*);
static void resolve_func(Stmt*);
static void resolve_var_decl(Stmt*);
static void resolve_if_stmt(Stmt*);
static void resolve_if_branch(IfBranch*, IfBranchType);
static void resolve_expr_stmt(Stmt*);
static DataType* make_data_type(const char*, u8);
static DataType* resolve_expr(Expr*);
static DataType* resolve_func_call(Expr*);
static DataType* resolve_set_expr(Expr*);
static DataType* resolve_arithmetic_expr(Expr*);
static DataType* resolve_comparison_expr(Expr*);
static DataType* resolve_variable_expr(Expr*);
static DataType* resolve_number_expr(Expr*);

static void init_data_types(void);
static DataType* make_data_type(const char*, u8);
static Token* make_token_from_string(const char*);
static bool data_type_match(DataType*, DataType*);
static char* data_type_to_string(DataType*);

#define CHECK_ERROR uint current_error = error_count

#define EXIT_ERROR(x) if (error_count > current_error) return x
#define EXIT_ERROR_VOID_RETURN if (error_count > current_error) return

void resolve_init(Stmt*** stmts_buf) {
	stmts_all = stmts_buf;
	data_type_strings = null;
	error_occured = false;
	persistent_error_occured = false;
	error_count = 0;

	init_data_types();
}

error_code resolve_run(void) {
	for (u64 i = 0; i < buf_len(stmts_all); ++i) {
		resolve_file(stmts_all[i]);
	}
	resolve_destroy();

	return persistent_error_occured || error_occured;
}

static void resolve_destroy(void) {
	for (u64 i = 0; i < buf_len(data_type_strings); ++i) {
		free(data_type_strings[i]);
	}
	buf_free(data_type_strings);
}

static void resolve_file(Stmt** stmts) {
	for (u64 i = 0; i < buf_len(stmts); ++i) {
		resolve_stmt(stmts[i]);
	}	
}

static void resolve_stmt(Stmt* stmt) {
	if (error_occured) {
		persistent_error_occured = error_occured;
		error_occured = false;
	}
	
	switch (stmt->type) {
		case STMT_FUNC: resolve_func(stmt); break;
		case STMT_VAR_DECL: resolve_var_decl(stmt); break;
		case STMT_IF: resolve_if_stmt(stmt); break;	
		case STMT_EXPR: resolve_expr_stmt(stmt); break;
		case STMT_STRUCT: break;	
	}
}

static void resolve_func(Stmt* stmt) {
	for (u64 i = 0; i < buf_len(stmt->func.body); ++i) {
		resolve_stmt(stmt->func.body[i]);
	}
}

static void resolve_var_decl(Stmt* stmt) {
	if (stmt->var_decl.initializer) {
		CHECK_ERROR;
		DataType* defined_type = stmt->var_decl.type;
		DataType* initializer_type = resolve_expr(stmt->var_decl.initializer);
		EXIT_ERROR_VOID_RETURN;
		
		if (!data_type_match(defined_type, initializer_type)) {
			error(stmt->var_decl.initializer->head,
				  "cannot initialize variable type '%s' from intializer "
				  "expression type '%s';",
				  data_type_to_string(defined_type),
				  data_type_to_string(initializer_type));
			return;
		}
	}
}

static void resolve_if_stmt(Stmt* stmt) {
	resolve_if_branch(stmt->if_stmt.if_branch, IF_IF_BRANCH);

	for (u64 i = 0; i < buf_len(stmt->if_stmt.elif_branch); ++i) {
		resolve_if_branch(stmt->if_stmt.elif_branch[i], IF_ELIF_BRANCH);
	}

	if (stmt->if_stmt.else_branch) {
		resolve_if_branch(stmt->if_stmt.else_branch, IF_ELSE_BRANCH);
	}
}

static void resolve_if_branch(IfBranch* branch, IfBranchType type) {
	if (type != IF_ELSE_BRANCH) {
		CHECK_ERROR;
		DataType* expr_type = resolve_expr(branch->cond);
		EXIT_ERROR_VOID_RETURN;
		
		if (!data_type_match(expr_type, bool_data_type)) {
			error(branch->cond->head,
				  "expected 'bool' data type in 'if' condition expression, "
				  "but got '%s';", data_type_to_string(expr_type));
		}
	}

	for (u64 i = 0; i < buf_len(branch->body); ++i) {
		resolve_stmt(branch->body[i]);
	}
}

static void resolve_expr_stmt(Stmt* stmt) {
	resolve_expr(stmt->expr);
}

static DataType* resolve_expr(Expr* expr) {
	switch (expr->type) {
		case EXPR_NUMBER:	 return resolve_number_expr(expr);
		case EXPR_STRING:	 return string_data_type;
		case EXPR_VARIABLE:  return resolve_variable_expr(expr);
		case EXPR_FUNC_CALL: return resolve_func_call(expr);
	}
	assert(0);
	return null;
}

static DataType* resolve_func_call(Expr* expr) {
	if (expr->func_call.callee->type == TOKEN_IDENTIFIER) {
		Stmt* function_called = expr->func_call.function_called;
		Stmt** params = function_called->func.params;
		Expr** args = expr->func_call.args;

		bool did_error_occur = false;
		for (u64 i = 0; i < buf_len(args); ++i) {
			CHECK_ERROR;
			DataType* param_type = params[i]->var_decl.type;
			DataType* arg_type = resolve_expr(args[i]);
			EXIT_ERROR(null);

			/*  TODO: match all data types, and then return */
			if (!data_type_match(param_type, arg_type)) {
				error(args[i]->head,
					  "expected type '%s', but got '%s';",
					  data_type_to_string(param_type),
					  data_type_to_string(arg_type));
				note(function_called->func.identifier,
					 "function '%s' defined here: ",
					 function_called->func.identifier->lexeme);
				/* TODO: check if we have to return null here */
				did_error_occur = true;
			}
		}
		if (did_error_occur) {
			/* TODO: check if we have to return null here */
			return function_called->func.type;
		}
		return function_called->func.type;
	}

	else if (expr->func_call.callee->type == TOKEN_KEYWORD) {
		if (str_intern(expr->func_call.callee->lexeme) ==
			str_intern("set")) {
			return resolve_set_expr(expr);
		}
	}

	else {
		switch (expr->func_call.callee->type) {
			case TOKEN_PLUS:
			case TOKEN_MINUS:
			case TOKEN_STAR:
			case TOKEN_SLASH: return resolve_arithmetic_expr(expr);

			case TOKEN_EQUAL: return resolve_comparison_expr(expr);
				
			default: return null;	
		}
	}
	return null;
}

static DataType* resolve_set_expr(Expr* expr) {
	CHECK_ERROR;
	DataType* var_type = resolve_variable_expr(expr->func_call.args[0]);
	DataType* expr_type = resolve_expr(expr->func_call.args[1]);
	EXIT_ERROR(null);
	
	if (!data_type_match(var_type, expr_type)) {
		Stmt* var_decl = expr->func_call.args[0]->variable.variable_decl_referenced;
		error(expr->func_call.args[1]->head,
			  "cannot set variable type '%s' to expression type '%s'",
			  data_type_to_string(var_type),
			  data_type_to_string(expr_type));
		note(var_decl->var_decl.identifier,
			 "variable '%s' declared here: ",
			 var_decl->var_decl.identifier->lexeme);
		/* TODO: we can also give a note if the expr is of type 
		 * EXPR_FUNC_CALL, and have it reference the line where the function
		 * was declared */
		/* TODO: check what to return here */
		return null;
	}
	return var_type;
}

static DataType* resolve_arithmetic_expr(Expr* expr) {
	Expr** args = expr->func_call.args;

	bool did_error_occur = false;
	for (u64 i = 0; i < buf_len(args); ++i) {
		DataType* arg_type = resolve_expr(args[i]);
		if (arg_type != null) {
			if (!data_type_match(arg_type, int_data_type)) {
				error(args[i]->head,
					  "'%s' operator can only operate on type '%s', but "
					  "got expression type '%s'",
					  expr->func_call.callee->lexeme,
					  "int",
					  data_type_to_string(arg_type));
				did_error_occur = true;
			}
		}
		else did_error_occur = true;
	}
	
	if (did_error_occur) {
		return null;
	}
	return int_data_type;
}

static DataType* resolve_comparison_expr(Expr* expr) {
	CHECK_ERROR;
	DataType* a_expr_type = resolve_expr(expr->func_call.args[0]);
	DataType* b_expr_type = resolve_expr(expr->func_call.args[1]);
	EXIT_ERROR(null);
	
	if (!data_type_match(a_expr_type, b_expr_type)) {
		error(expr->func_call.args[1]->head,
			  "'%s': cannot compare conflicting types '%s' and '%s'",
			  expr->func_call.callee->lexeme,
			  data_type_to_string(a_expr_type),
			  data_type_to_string(b_expr_type));
		return null;
	}
	return bool_data_type;
}

static DataType* resolve_variable_expr(Expr* expr) {
	return expr->variable.variable_decl_referenced->var_decl.type;
}

static DataType* resolve_number_expr(Expr* expr) {
	/* TODO: return float depending on literal */
	return int_data_type;	
}

static void init_data_types(void) {
	int_data_type = make_data_type("int", 0);
	string_data_type = make_data_type("char", 1);
	bool_data_type = make_data_type("bool", 0);
}

static DataType* make_data_type(const char* main_type, u8 pointer_count) {
	DataType* type = (DataType*)malloc(sizeof(DataType));
	type->type = make_token_from_string(main_type);
	type->pointer_count = pointer_count;
	/* TODO: ??? push type into buf to free it later */
	return type;
}

static Token* make_token_from_string(const char* str) {
	Token* t = (Token*)malloc(sizeof(Token));
	t->type = TOKEN_KEYWORD; /* TODO: does it need to be KEYWORD? */
	t->lexeme = (char*)str_intern((char*)str);
	t->line = 0;
	t->column = 0;
	/* TODO: ??? push type into buf to free it later */
	return t;
}

static bool data_type_match(DataType* a, DataType* b) {
	if (a && b) {
		if (is_token_identical(a->type, b->type)) {
			if (a->pointer_count == b->pointer_count) {
				return true;
			}
		}
	}
	return false;
}

static char* data_type_to_string(DataType* type) {
	/* TODO: this is highly inefficient; use maps */
	u64 main_type_len = strlen(type->type->lexeme);
	u64 len = main_type_len + type->pointer_count;
	char* str = (char*)malloc(len + 1);

	strncpy(str, type->type->lexeme, main_type_len);
	for (u64 i = main_type_len; i < len; ++i) {
		str[i] = '*';
	}
	str[len] = '\0';
	buf_push(data_type_strings, str);
	return str;
}

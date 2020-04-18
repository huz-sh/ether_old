#include <ether/ether.h>
#include <ether/linker_resolve_code_gen_common.h>

#define DATA_TYPE_MATCH 1
#define DATA_TYPE_IMPLICIT_MATCH 2
#define DATA_TYPE_NOT_MATCH 0

static Stmt** stmts;
static Stmt** structs;
static char** data_type_strings;
static DataType** cloned_data_types;
static bool error_occured;
static bool persistent_error_occured;
static uint error_count;

static DataType* int_data_type;
static DataType* char_data_type;
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
static DataType* resolve_dot_access_expr(Expr*);
static DataType* resolve_func_call(Expr*);
static DataType* resolve_set_expr(Expr*);
static DataType* resolve_deref_expr(Expr*);
static DataType* resolve_addr_expr(Expr*);
static DataType* resolve_at_expr(Expr*); 
static DataType* resolve_arithmetic_expr(Expr*);
static DataType* resolve_comparison_expr(Expr*);
static DataType* resolve_variable_expr(Expr*);
static DataType* resolve_number_expr(Expr*);

static void init_data_types(void);
static Stmt* find_struct_by_name(char*);
static DataType* make_data_type(const char*, u8);
static DataType* clone_data_type(DataType*);
static Token* make_token_from_string(const char*);
static int data_type_match(DataType*, DataType*);
static bool is_one_token(const char*, Token*, Token*);
static char* data_type_to_string(DataType*);
static DataType* get_smaller_type(DataType*, DataType*);
static u64 get_data_type_size(DataType*);
static void implicit_cast_warning(Token*, DataType*, DataType*);

#define CHECK_ERROR uint current_error = error_count

#define EXIT_ERROR(x) if (error_count > current_error) return x
#define EXIT_ERROR_VOID_RETURN if (error_count > current_error) return

void resolve_init(Stmt** p_stmts, Stmt** p_structs) {
	stmts = p_stmts;
	structs = p_structs;
	data_type_strings = null;
	error_occured = false;
	persistent_error_occured = false;
	error_count = 0;

	init_data_types();
}

error_code resolve_run(void) {
	resolve_file(stmts);
	resolve_destroy();

	return persistent_error_occured || error_occured;
}

static void resolve_destroy(void) {
	for (u64 i = 0; i < buf_len(data_type_strings); ++i) {
		free(data_type_strings[i]);
	}
	for (u64 i = 0; i < buf_len(cloned_data_types); ++i) {
		free(cloned_data_types[i]);
	}
	buf_free(data_type_strings);
	buf_free(cloned_data_types);
}

static void resolve_file(Stmt** p_stmts) {
	for (u64 i = 0; i < buf_len(p_stmts); ++i) {
		resolve_stmt(p_stmts[i]);
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
	if (!stmt->func.is_function) {
		return;
	}
	
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

		int match = data_type_match(defined_type, initializer_type);
		if (match == DATA_TYPE_NOT_MATCH) {
			error(stmt->var_decl.initializer->head,
				  "cannot initialize variable type '%s' from intializer "
				  "expression type '%s';",
				  data_type_to_string(defined_type),
				  data_type_to_string(initializer_type));
			return;
		}
		else if (match == DATA_TYPE_IMPLICIT_MATCH) {
			implicit_cast_warning(stmt->var_decl.initializer->head,
								  initializer_type, defined_type);
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

		int match = data_type_match(expr_type, bool_data_type);
		if (match == DATA_TYPE_NOT_MATCH) {
			error(branch->cond->head,
				  "expected 'bool' data type in 'if' condition expression, "
				  "but got '%s';", data_type_to_string(expr_type));
		}
		else if (match == DATA_TYPE_IMPLICIT_MATCH) {
			warning(branch->cond->head,
					"implicit cast to 'bool' from '%s' in 'if' condition expression;",
					data_type_to_string(expr_type));
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
		case EXPR_DOT_ACCESS:	return resolve_dot_access_expr(expr);
		case EXPR_NUMBER:	 	return resolve_number_expr(expr);
		case EXPR_STRING:	 	return string_data_type;
		case EXPR_CHAR:		 	return char_data_type;	
		case EXPR_VARIABLE:  	return resolve_variable_expr(expr);
		case EXPR_FUNC_CALL: 	return resolve_func_call(expr);
	}
	assert(0);
	return null;
}

static DataType* resolve_dot_access_expr(Expr* expr) {
	CHECK_ERROR;
	DataType* left_type = resolve_expr(expr->dot.left);
	EXIT_ERROR(null);

	if (!left_type) return null;

	if (left_type->type->type != TOKEN_KEYWORD) {
		if (left_type->pointer_count == 0 ||
			left_type->pointer_count == 1) {
			expr->dot.is_left_pointer = (left_type->pointer_count == 0 ?
										 false : true);
			Stmt* struct_ref = find_struct_by_name(left_type->type->lexeme);
			if (struct_ref) {
				for (u64 f = 0; f < buf_len(struct_ref->struct_stmt.fields); ++f) {
					if (is_token_identical(struct_ref->struct_stmt.fields[f]->identifier,
										   expr->dot.right)) {
						return struct_ref->struct_stmt.fields[f]->type;
					}
				}

				error(expr->dot.right,
					  "cannot find field '%s' in struct '%s';",
					  expr->dot.right->lexeme,
					  left_type->type->lexeme);
				note(struct_ref->struct_stmt.identifier,
					 "struct '%s' defined here: ",
					 left_type->type->lexeme);
				return null;
			}
		}
	}
	error(expr->dot.left->head,
		  "invalid operand to '.' operator: '%s'; use 'deref' operator instead;",
		  data_type_to_string(left_type));
	return null;
}

static DataType* resolve_func_call(Expr* expr) {
	if (expr->func_call.callee->type == TOKEN_IDENTIFIER &&
		expr->func_call.function_called) {
		Stmt* function_called = expr->func_call.function_called;
		Stmt** params = function_called->func.params;
		Expr** args = expr->func_call.args;

		bool did_error_occur = false;
		for (u64 i = 0; i < buf_len(args); ++i) {
			CHECK_ERROR;
			DataType* param_type = params[i]->var_decl.type;
			DataType* arg_type = resolve_expr(args[i]);
			EXIT_ERROR(null);

			int match = data_type_match(param_type, arg_type);
			if (match == DATA_TYPE_NOT_MATCH) {
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
			else if (match == DATA_TYPE_IMPLICIT_MATCH) {
				implicit_cast_warning(args[i]->head,
									  param_type,
									  arg_type);
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
		else if (str_intern(expr->func_call.callee->lexeme) ==
				 str_intern("deref")) {
			return resolve_deref_expr(expr);
		}
		else if (str_intern(expr->func_call.callee->lexeme) ==
				 str_intern("addr")) {
			return resolve_addr_expr(expr);
		}
		else if (str_intern(expr->func_call.callee->lexeme) ==
				 str_intern("at")) {
			return resolve_at_expr(expr);
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
	DataType* var_type = resolve_expr(expr->func_call.args[0]);
	DataType* expr_type = resolve_expr(expr->func_call.args[1]);
	EXIT_ERROR(null);

	int match = data_type_match(var_type, expr_type);
	if (match == DATA_TYPE_NOT_MATCH) {
		error(expr->func_call.args[1]->head,
			  "cannot set variable type '%s' to expression type '%s'",
			  data_type_to_string(var_type),
			  data_type_to_string(expr_type));
		return null;
	}
	else if (match == DATA_TYPE_IMPLICIT_MATCH) {
		implicit_cast_warning(expr->func_call.args[1]->head,
							  expr_type,
							  var_type);
	}
	return var_type;
}

static DataType* resolve_deref_expr(Expr* expr) {
	CHECK_ERROR;
	DataType* type = resolve_expr(expr->func_call.args[0]);
	EXIT_ERROR(null);

	if (type->pointer_count == 0) {
		error(expr->func_call.args[0]->head,
			  "cannot dereference a non-pointer type;");
		return null;
	}
	else if	((str_intern(type->type->lexeme) == str_intern("void") &&
		 type->pointer_count == 1)) {
		error(expr->func_call.args[0]->head,
			  "cannot dereference a void-pointer; nameless type;");
		return null;
	}

	DataType* dereferenced_type = clone_data_type(type);
	dereferenced_type->pointer_count--;
	return dereferenced_type;
}

static DataType* resolve_addr_expr(Expr* expr) {
	CHECK_ERROR;
	DataType* type = resolve_expr(expr->func_call.args[0]);
	EXIT_ERROR(null);

	DataType* addressed_type = clone_data_type(type);
	addressed_type->pointer_count++;
	return addressed_type;
}

static DataType* resolve_at_expr(Expr* expr) {
	CHECK_ERROR;
	DataType* var_type = resolve_expr(expr->func_call.args[0]);
	DataType* expr_type = resolve_expr(expr->func_call.args[1]);
	EXIT_ERROR(null);

	int match = data_type_match(int_data_type, expr_type);
	if (match == DATA_TYPE_NOT_MATCH) {
		error(expr->func_call.args[1]->head,
			  "conflicting types (second param): expected an integer here: ");
		/* TODO: check what to return here */
		return null;
	}
	else if (match == DATA_TYPE_IMPLICIT_MATCH) {
		implicit_cast_warning(expr->func_call.args[1]->head,
							  expr_type,
							  int_data_type);
	}

	DataType* at_type = clone_data_type(var_type);
	at_type->pointer_count--;
	return at_type;
}

static DataType* resolve_arithmetic_expr(Expr* expr) {
	Expr** args = expr->func_call.args;

	bool did_error_occur = false;
	for (u64 i = 0; i < buf_len(args); ++i) {
		DataType* arg_type = resolve_expr(args[i]);
		if (arg_type != null) {
			/* TODO: if all args are of same integer type, then
			 * no warnings are to be outputted */
			int match = data_type_match(arg_type, int_data_type);
			if (match == DATA_TYPE_NOT_MATCH) {
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

	int match = data_type_match(a_expr_type, b_expr_type);
	if (match == DATA_TYPE_NOT_MATCH) {
		error(expr->func_call.args[1]->head,
			  "'%s': cannot compare conflicting types '%s' and '%s'",
			  expr->func_call.callee->lexeme,
			  data_type_to_string(a_expr_type),
			  data_type_to_string(b_expr_type));
		return null;
	}
	else if (match == DATA_TYPE_IMPLICIT_MATCH) {
		DataType* smaller_type = get_smaller_type(a_expr_type, b_expr_type);
		Token* error_token = null;
		bool a_is_smaller = false;

		if (smaller_type == a_expr_type) {
			a_is_smaller = true;
			error_token = expr->func_call.args[0]->head;
		}
		else if (smaller_type == b_expr_type) {
			error_token = expr->func_call.args[0]->head;
		}
		else assert(0);

		warning(error_token,
				"implicit cast from '%s' to '%s';",
				data_type_to_string(smaller_type),
				data_type_to_string((a_is_smaller ?
									 b_expr_type :
									 a_expr_type)));

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
	char_data_type = make_data_type("char", 0);
	string_data_type = make_data_type("char", 1);
	bool_data_type = make_data_type("bool", 0);
}

static Stmt* find_struct_by_name(char* name) {
	for (u64 i = 0; i < buf_len(structs); ++i) {
		if (str_intern(structs[i]->struct_stmt.identifier->lexeme) ==
			str_intern(name)) {
			return structs[i];
		}
	}
	assert(0);
	return null;
}

static DataType* make_data_type(const char* main_type, u8 pointer_count) {
	DataType* type = (DataType*)malloc(sizeof(DataType));
	type->type = make_token_from_string(main_type);
	type->pointer_count = pointer_count;
	/* TODO: ??? push type into buf to free it later */
	return type;
}

static DataType* clone_data_type(DataType* p_type) {
	DataType* type = (DataType*)malloc(sizeof(DataType));
	type->type = p_type->type;
	type->pointer_count = p_type->pointer_count;
	buf_push(cloned_data_types, type);
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

static int data_type_match(DataType* a, DataType* b) {
	if (a && b) {
		if (a->pointer_count != b->pointer_count) {
			return DATA_TYPE_NOT_MATCH;
		}

		bool main_data_type_identical = is_token_identical(a->type, b->type);
		if (a->pointer_count > 0) {
			if (main_data_type_identical) {
				return DATA_TYPE_MATCH;
			}
			return DATA_TYPE_IMPLICIT_MATCH;
		}
			
		if (main_data_type_identical) {
			return DATA_TYPE_MATCH;
		}
		else {
			/* implicit non-pointer cast */
			if (is_one_token("int", a->type, b->type) &&
				is_one_token("char", a->type, b->type)) {
				return DATA_TYPE_IMPLICIT_MATCH;
			}
		}
	}
	return DATA_TYPE_NOT_MATCH;
}

static bool is_one_token(const char* equal, Token* a, Token* b) {
	if (str_intern(a->lexeme) == str_intern((char*)equal)) {
		return true;
	}
	if (str_intern(b->lexeme) == str_intern((char*)equal)) {
		return true;
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

static DataType* get_smaller_type(DataType* a, DataType* b) {
	u64 a_size = get_data_type_size(a);
	u64 b_size = get_data_type_size(b);

	if (a_size <= b_size) {
		return a;
	}
	return b;
}

static u64 get_data_type_size(DataType* type) {
	if (type->pointer_count > 0) {
		return sizeof(void*);
	}
	else {
		if (str_intern(type->type->lexeme) ==
			str_intern("int")) {
			return sizeof(int);
		}
		else if (str_intern(type->type->lexeme) ==
				 str_intern("char")) {
			return sizeof(char);
		}
		else if (str_intern(type->type->lexeme) ==
				 str_intern("bool")) {
			return sizeof(u8); /* bool takes a bytes in Ether */
		}
		else {
			return 16; /* TODO: remove constant and compute size for custom types */
			/* 16 here is temporary. */
		}
	}
}

static void implicit_cast_warning(Token* error_token,
								  DataType* a, DataType* b) {
	warning(error_token,
			"implicit cast from '%s' to '%s';",
			data_type_to_string(a),
			data_type_to_string(b));	
}

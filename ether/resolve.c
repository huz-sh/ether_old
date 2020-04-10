#include <ether/ether.h>
#include <ether/linker_resolve_common.h>

static Stmt*** stmts_all;
static char** data_type_strings;
static bool error_occured;
static uint error_count;

static DataType* int_data_type;

static void resolve_destroy(void);

static void resolve_file(Stmt**);
static void resolve_stmt(Stmt*);
static void resolve_func(Stmt*);
static void resolve_var_decl(Stmt*);
static void resolve_expr_stmt(Stmt*);
static DataType* make_data_type(const char*, u8);
static DataType* resolve_expr(Expr*);
static DataType* resolve_func_call(Expr*);
static DataType* resolve_variable_expr(Expr*);
static DataType* resolve_number_expr(Expr*);

static void init_data_types(void);
static DataType* make_data_type(const char* main_type, u8 pointer_count);
static Token* make_token_from_string(const char*);
static bool data_type_match(DataType*, DataType*);
static char* data_type_to_string(DataType* type);

void resolve_init(Stmt*** stmts_buf) {
	stmts_all = stmts_buf;
	data_type_strings = null;
	error_occured = false;
	error_count = 0;

	init_data_types();
}

error_code resolve_run(void) {
	for (u64 i = 0; i < buf_len(stmts_all); ++i) {
		resolve_file(stmts_all[i]);
	}
	resolve_destroy();

	return error_occured;
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
	switch (stmt->type) {
		case STMT_FUNC: resolve_func(stmt); break;
		case STMT_VAR_DECL: resolve_var_decl(stmt); break;
		case STMT_EXPR: resolve_expr_stmt(stmt); break;	
	}
}

static void resolve_func(Stmt* stmt) {
	for (u64 i = 0; i < buf_len(stmt->func.body); ++i) {
		resolve_stmt(stmt->func.body[i]);
	}
}

static void resolve_var_decl(Stmt* stmt) {
	if (stmt->var_decl.initializer) {
		DataType* defined_type = stmt->var_decl.type;
		DataType* initializer_type = resolve_expr(stmt->var_decl.initializer);
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

static void resolve_expr_stmt(Stmt* stmt) {
	resolve_expr(stmt->expr);
}

static DataType* resolve_expr(Expr* expr) {
	switch (expr->type) {
		case EXPR_FUNC_CALL: return resolve_func_call(expr);
		case EXPR_VARIABLE:  return resolve_variable_expr(expr);
		case EXPR_NUMBER:	 return resolve_number_expr(expr);	
	}
	assert(0);
	return null;
}

static DataType* resolve_func_call(Expr* expr) {
	
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
	assert(a);
	assert(b);

	if (is_token_identical(a->type, b->type)) {
		if (a->pointer_count == b->pointer_count) {
			return true;
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

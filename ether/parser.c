#include <ether/ether.h>

static Token** tokens;
static u64 tokens_len;
static Stmt** stmts;
static char** built_in_data_types;

static u64 idx;
static uint error_count;
static bool error_occured;
static uint error_bracket_counter;
static bool error_panic;
static bool start_stmt_bracket;

static void parser_destroy(void);

static Stmt* parse_decl(void);
static Stmt* parse_stmt(void);
static Stmt* parse_struct(Token*);
static Stmt* parse_func(DataType*, Token*);
static Stmt* parse_var_decl(DataType*, Token*);
static Stmt* parse_expr_stmt(void);

static Expr* parse_expr(void);
static Expr* parse_primary_expr(void);
static Expr* parse_func_call_expr(void);

static Expr* make_number_expr(Token*);
static Expr* make_variable_expr(Token*);
static Expr* make_func_call_expr(Token*, Expr**);

static bool match_token_type(TokenType);
inline static bool match_left_bracket(void);
inline static bool match_right_bracket(void);
static bool match_keyword(char*);
static DataType* match_data_type(void);
static bool peek(TokenType);
static void expect_token_type(TokenType, const char*, ...);

inline static void consume_left_bracket(void);
inline static void consume_right_bracket(void);
inline static void consume_colon(void);
inline static Token* consume_identifier(void);
static DataType* consume_data_type(void);

static void goto_next_token(void);
static void goto_previous_token(void);
static Token* current(void);
static Token* previous(void);

static void error_at_current(const char*, ...);
static void error(Token*, const char*, ...);
static void warning_at_previous(const char*, ...);
static void warning(Token*, const char*, ...);
static void sync_to_next_statement(void);

#define CUR_ERROR uint current_error_count = error_count
#define EXIT_ERROR if (error_count > current_error_count) return

#define CHECK_EOF(x) \
if (current()->type == TOKEN_EOF) { \
	error_at_current("end of file while parsing function body; did you forget a ']'?"); \
	return (x);																\
}

void parser_init(Token** tokens_buf) {
	tokens = tokens_buf;
	tokens_len = buf_len(tokens_buf);
	idx = 0;
	error_count = 0;
	error_occured = false;
	error_bracket_counter = 0;
	error_panic = false;
	start_stmt_bracket = false;

	buf_push(built_in_data_types, str_intern("int"));
	buf_push(built_in_data_types, str_intern("char"));
	buf_push(built_in_data_types, str_intern("void"));
}

Stmt** parser_run(error_code* out_error_code) {
	while (current()->type != TOKEN_EOF) {
		Stmt* stmt = parse_decl();
		if (stmt) buf_push(stmts, stmt);
	}
	if (out_error_code) *out_error_code = error_occured;
	parser_destroy();
	return stmts;
}

void parser_destroy(void) {
	buf_free(built_in_data_types);
}

/* only top level statements (functions, structs, var) */
static Stmt* parse_decl(void) {
	consume_left_bracket();

	Stmt* stmt = null;
	if (match_keyword("struct")) {
		Token* identifier = consume_identifier();
		stmt = parse_struct(identifier);
	}
	else if (match_keyword("let")) {
		DataType* type = consume_data_type();
		consume_colon();
		Token* identifier = consume_identifier();
		stmt = parse_var_decl(type, identifier);
		consume_right_bracket();
	}
	else {
		DataType* type = consume_data_type();
		consume_colon();
		Token* identifier = consume_identifier();
		consume_left_bracket();
		stmt = parse_func(type, identifier);
	}

	return stmt;
}

/* only statements inside scope / function */
static Stmt* parse_stmt(void) {
	start_stmt_bracket = true;
	consume_left_bracket();
	start_stmt_bracket = false;

	Stmt* stmt = null;
	if (match_keyword("let")) {
		DataType* dt = consume_data_type();
		consume_colon();
		Token* identifier = consume_identifier();
		stmt = parse_var_decl(dt, identifier);
		consume_right_bracket();
	}
	else {
		goto_previous_token();
		stmt = parse_expr_stmt();
	}

	error_panic = false;
	return stmt;
}

#define MAKE_STMT(x) Stmt* x = (Stmt*)malloc(sizeof(Stmt));

static Stmt* parse_struct(Token* identifier) {
	Field** fields = null;
	if (!match_right_bracket()) {
		do {
			consume_left_bracket();
			DataType* d = consume_data_type();
			consume_colon();
			Token* t = consume_identifier();

			Field* f = (Field*)malloc(sizeof(Field));
			f->type = d;
			f->identifier = t;
			buf_push(fields, f);
			consume_right_bracket();
			CHECK_EOF(null);
		} while (!match_right_bracket());
	}

	MAKE_STMT(new);
	new->type = STMT_STRUCT;
	new->struct_stmt.identifier = identifier;
	new->struct_stmt.fields = fields;
	return new;
}

static Stmt* parse_func(DataType* d, Token* t) {
	Param** params = null;
	if (match_keyword("void")) {
		consume_right_bracket();
	}
	else if (peek(TOKEN_RIGHT_BRACKET)) {
		/* we call warning at 'previous' because we want to
		 * mark the left bracket as the cause */
		warning_at_previous("empty function parameter list here:");
		goto_next_token();
	}
	else {
		do {
			CUR_ERROR;
			DataType* p_type = consume_data_type();
			consume_colon();
			Token* p_name = consume_identifier();

			Param* param = (Param*)malloc(sizeof(Param));
			param->type = p_type;
			param->identifier = p_name;
			buf_push(params, param);
			EXIT_ERROR null;
			CHECK_EOF(null);
		} while (!match_right_bracket());
	}

	Stmt** body = null;
	while (!match_right_bracket()) {
		CUR_ERROR;
		CHECK_EOF(null);
		Stmt* stmt = parse_stmt();
		if (stmt) buf_push(body, stmt);
		EXIT_ERROR null;
	}

	MAKE_STMT(new);
	new->type = STMT_FUNC;
	new->func.type = d;
	new->func.identifier = t;
	new->func.params = params;
	new->func.body = body;
	return new;
}

static Stmt* parse_var_decl(DataType* d, Token* t) {
	Expr* init = null;
	if (match_token_type(TOKEN_EQUAL)) {
		init = parse_expr();
	}

	MAKE_STMT(new);
	new->type = STMT_VAR_DECL;
	new->var_decl.type = d;
	new->var_decl.identifier = t;
	new->var_decl.initializer = init;
	return new;
}

static Stmt* parse_expr_stmt(void) {
	MAKE_STMT(new);
	Expr* e = parse_expr();
	new->type = STMT_EXPR;
	new->expr = e;
	return new;
}

static Expr* parse_expr(void) {
	return parse_primary_expr();
}

static Expr* parse_primary_expr(void) {
	if (match_token_type(TOKEN_NUMBER)) {
		return make_number_expr(previous());
	}
	else if (match_token_type(TOKEN_IDENTIFIER)) {
		return make_variable_expr(previous());
	}
	else if (match_left_bracket()) {
		return parse_func_call_expr();
	}
	else {
		error_at_current("invalid syntax; expected identifier, "
		 				 "literal, or grouping but got '%s'",
						 current()->lexeme);
	}
	return null;
}

static Expr* parse_func_call_expr(void) {
	Token* callee = null;
	switch (current()->type) {
		case TOKEN_IDENTIFIER:
		case TOKEN_PLUS:
		case TOKEN_MINUS:
		case TOKEN_STAR:
		case TOKEN_SLASH:
		case TOKEN_EQUAL: {
			callee = current();
			goto_next_token();
		} break;

		default: {
			/* TODO: refactor into a buf if added more function operators */
			if (match_keyword("set")) {
				callee = previous();
			}
			else {
				error_at_current("expected identifier or operator here:");
				return null;
			}
		}
	}

	Expr** args = null;
	if (!match_right_bracket()) {
		do {
			CUR_ERROR;
			Expr* e = parse_expr();
			if (e) buf_push(args, e);
			CHECK_EOF(null);
			EXIT_ERROR null;
		} while (!match_right_bracket());
	}

	return make_func_call_expr(callee, args);
}

#define MAKE_EXPR(x) Expr* x = (Expr*)malloc(sizeof(Expr));

static Expr* make_number_expr(Token* t) {
	MAKE_EXPR(new);
	new->type = EXPR_NUMBER;
	new->number = t;
	return new;
}

static Expr* make_variable_expr(Token* t) {
	MAKE_EXPR(new);
	new->type = EXPR_VARIABLE;
	new->number = t;
	return new;
}


static Expr* make_func_call_expr(Token* callee, Expr** args) {
	MAKE_EXPR(new);
	new->type = EXPR_FUNC_CALL;
	new->func_call.callee = callee;
	new->func_call.args = args;
	return new;
}

static bool match_token_type(TokenType t) {
	if (peek(t)) {
		goto_next_token();
		return true;
	}
	return false;
}

static bool match_left_bracket(void) {
	if (match_token_type(TOKEN_LEFT_BRACKET)) {
		return true;
	}
	return false;
}

static bool match_right_bracket(void) {
	if (match_token_type(TOKEN_RIGHT_BRACKET)) {
		return true;
	}
	return false;
}

static bool match_keyword(char* s) {
	if (peek(TOKEN_KEYWORD) &&
		str_intern(current()->lexeme) == str_intern(s)) {
		goto_next_token();
		return true;
	}
	return false;
}

static DataType* match_data_type(void) {
	DataType* new = null;
	bool matched_main_type = false;
	if (match_token_type(TOKEN_IDENTIFIER)) {
		matched_main_type = true;
		new = (DataType*)malloc(sizeof(DataType));
		new->type = previous();
	}
	else {
		for (uint i = 0; i < buf_len(built_in_data_types); ++i) {
			if (match_keyword(built_in_data_types[i])) {
				matched_main_type = true;
				new = (DataType*)malloc(sizeof(DataType));
				new->type = previous();
			}
		}
	}

	if (matched_main_type) {
		u8 pointer_count = 0;
		while (match_token_type(TOKEN_STAR)) {
			pointer_count++;
		}
		new->pointer_count = pointer_count;
	}

	return new;
}

static bool peek(TokenType t) {
	if (idx >= tokens_len) {
		return false;
	}

	if (current()->type == t) {
		return true;
	}
	return false;
}

static void expect_token_type(TokenType t, const char* msg, ...) {
	if (match_token_type(t)) {
		return;
	}
	va_list ap;
	va_start(ap, msg);
	error_at_current(msg, ap);
	va_end(ap);
}

inline static void consume_left_bracket(void) {
	if (!start_stmt_bracket) error_bracket_counter++;
	expect_token_type(TOKEN_LEFT_BRACKET, "expected '[' here:");
}

inline static void consume_right_bracket(void) {
	expect_token_type(TOKEN_RIGHT_BRACKET, "expected ']' here:");
}

inline static void consume_colon(void) {
	expect_token_type(TOKEN_COLON, "expected ':' here:");
}

inline static Token* consume_identifier(void) {
	expect_token_type(TOKEN_IDENTIFIER, "expected identifier here:");
	return previous();
}

static DataType* consume_data_type(void) {
	DataType* type = match_data_type();
	if (!type) {
		error_at_current("expected data type here:");
	}
	return type;
}

static void goto_next_token(void) {
	if (idx == 0 || (idx - 1) < tokens_len) {
		++idx;
	}
}

static void goto_previous_token(void) {
	if (idx > 0) {
		--idx;
	}
}

static Token* current(void) {
	if (idx >= tokens_len) {
		return null;
	}
	return tokens[idx];
}

static Token* previous(void) {
	if (idx >= tokens_len + 1) {
		return null;
	}
	return tokens[idx - 1];
}

static void error_at_current(const char* msg, ...) {
	va_list ap;
	va_start(ap, msg);
	error(current(), msg, ap);
	va_end(ap);
}

static void error(Token* t, const char* msg, ...) {
	if (error_panic) return;
	error_panic = true;

	va_list ap;
	va_start(ap, msg);
	printf("%s:%ld:%d: error: ", t->srcfile->fpath, t->line, t->column);
	vprintf(msg, ap);
	va_end(ap);
	printf("\n");

	print_file_line_with_info(t->srcfile, t->line);
	print_marker_arrow_with_info_ln(t->srcfile, t->line, t->column);

	sync_to_next_statement();

	error_occured = ETHER_ERROR;
	++error_count;
}

static void warning_at_previous(const char* msg, ...) {
	va_list ap;
	va_start(ap, msg);
	warning(previous(), msg, ap);
	va_end(ap);
}

static void warning(Token* t, const char* msg, ...) {
	va_list ap;
	va_start(ap, msg);
	printf("%s:%ld:%d: warning: ", t->srcfile->fpath, t->line, t->column);
	vprintf(msg, ap);
	va_end(ap);
	printf("\n");

	print_file_line_with_info(t->srcfile, t->line);
	print_marker_arrow_with_info_ln(t->srcfile, t->line, t->column);	
}

static void sync_to_next_statement(void) {
	while (true) {
		if (current()->type == TOKEN_RIGHT_BRACKET) {
			if (error_bracket_counter == 0) {
				/* stmt bkt */
				goto_next_token();
				return;
			}
			else if (error_bracket_counter > 0) {
				error_bracket_counter--;
				goto_next_token();
			}
		}
		else if (current()->type == TOKEN_LEFT_BRACKET) {
			error_bracket_counter++;
			goto_next_token();
		}
		else {
			if (current()->type == TOKEN_EOF) {
				++error_count;
				return;
			}
			goto_next_token();
		}
	}
}

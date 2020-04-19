#include <ether/ether.h>

static Token** tokens;
static u64 tokens_len;
static Stmt** stmts;
static SourceFile* srcfile;

static char** built_in_data_types;
static char** operator_keywords;

static u64 idx;
static uint error_count;
static bool error_occured;
static uint error_bracket_counter;
static bool error_panic;
static bool start_stmt_bracket;
static Stmt* current_function;

static void parser_destroy(void);

static Stmt* parse_decl(void);
static Stmt* parse_stmt(void);
static Stmt* parse_struct(Token*);
static Stmt* parse_func(void);
static error_code parse_func_header(Stmt*, bool);
static Stmt* parse_func_decl(void);
static void parse_load_stmt(void);
static Stmt* parse_var_decl(DataType*, Token*, bool);
static Stmt* parse_if_stmt(void);
static void parse_if_branch(Stmt*, IfBranchType);
static Stmt* parse_return_stmt(Token*);
static Stmt* parse_expr_stmt(void);

static Expr* parse_expr(void);
static Expr* parse_dot_access_expr(void);
static Expr* parse_primary_expr(void);
static Expr* parse_func_call_expr(void);

static Expr* make_dot_access_expr(Expr*, Token*);
static Expr* make_number_expr(Token*);
static Expr* make_char_expr(Token*);
static Expr* make_string_expr(Token*);
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

static void init_built_in_data_types(void);
static void init_operator_keywords(void);
	
#define CUR_ERROR uint current_error_count = error_count
#define EXIT_ERROR if (error_count > current_error_count) return

#define CHECK_EOF(x) \
if (current()->type == TOKEN_EOF) { \
	error_at_current("end of file while parsing function body; did you forget a ']'?"); \
	return (x); \
}

#define CHECK_EOF_VOID_RETURN \
if (current()->type == TOKEN_EOF) { \
	error_at_current("end of file while parsing function body; did you forget a ']'?"); \
	return; \
}

void parser_init(Token** tokens_buf, SourceFile* file) {
	tokens = tokens_buf;
	tokens_len = buf_len(tokens_buf);
	srcfile = file;
	idx = 0;
	error_count = 0;
	error_occured = false;
	error_bracket_counter = 0;
	error_panic = false;
	start_stmt_bracket = false;
	current_function = null;

	init_built_in_data_types();
	init_operator_keywords();
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

	/* TODO: parser necessary data types and identifiers in functions 
	 * and leave this clean */
	Stmt* stmt = null;
	if (match_keyword("struct")) {
		Token* identifier = consume_identifier(); /* TODO: to refactor */
		stmt = parse_struct(identifier);
	}
	else if (match_keyword("let")) {
		DataType* type = consume_data_type(); /* TODO: to refactor */
		consume_colon();
		Token* identifier = consume_identifier();
		stmt = parse_var_decl(type, identifier, true);
	}
	else if (match_keyword("defn")) {
		stmt = parse_func();
	}
	else if (match_keyword("decl")) {
		stmt = parse_func_decl();
	}
	else if (match_keyword("load")) {
		parse_load_stmt();
		return null;
	}
	else {
		error_at_current("expected keyword 'load', 'struct', 'defn', 'decl' or 'let' "
						 "in global scope; did you miss a ']'?");
		return null;
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
		stmt = parse_var_decl(dt, identifier, false);
	}
	else if (match_keyword("return")) {
		stmt = parse_return_stmt(previous());
	}
	else if (match_keyword("if")) {
		stmt = parse_if_stmt();
	}
	else if (match_keyword("elif") || match_keyword("else")) {
		error(previous(), "'%s' branch without preceding 'if' statement; "
						  "did you mean 'if'?", previous()->lexeme);
		return null;
	}
	else if (match_keyword("struct")) {
		error(previous(), "cannot define a type inside a function-scope; "
						  "did you miss a ']'?");
		return null;
	}
	else if (match_keyword("defn")) {
		error(previous(), "cannot define a function inside a function-scope; "
						  "did you miss a ']'?");
		return null;
	}
	else if (match_keyword("decl")) {
		error(previous(), "cannot declare a function inside a function-scope; "
						  "did you miss a ']'?");
		return null;
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

static Stmt* parse_func(void) {
	MAKE_STMT(new);
	error_code header_parsing_error = parse_func_header(new, true);
	if (header_parsing_error != ETHER_SUCCESS) return null;
	current_function = new;

	Stmt** body = null;
	while (!match_right_bracket()) {
		CUR_ERROR;
		CHECK_EOF(null);
		Stmt* stmt = parse_stmt();
		if (stmt) buf_push(body, stmt);
		EXIT_ERROR null;
	}

	new->type = STMT_FUNC;
	new->func.body = body;
	return new;
}

static error_code parse_func_header(Stmt* stmt, bool is_function) {
	DataType* type = consume_data_type(); 
	consume_colon();
	Token* identifier = consume_identifier();
	consume_left_bracket();
	
	Stmt** params = null;
	bool has_params = true;

	if (current()->type == TOKEN_KEYWORD) {
		if (str_intern(current()->lexeme) ==
			str_intern("void")) {
			
			if (idx < tokens_len-1 && tokens[idx+1]->type == TOKEN_RIGHT_BRACKET) {
				has_params = false;
			}
		}
	}
	
	if (!has_params) {
		goto_next_token();
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

			MAKE_STMT(param);
			param->type = STMT_VAR_DECL;
			param->var_decl.type = p_type;
			param->var_decl.identifier = p_name;
			param->var_decl.initializer = null;
			param->var_decl.is_global_var = false;
			buf_push(params, param);
			
			EXIT_ERROR ETHER_ERROR;
			CHECK_EOF(ETHER_ERROR);
		} while (!match_right_bracket());
	}

	stmt->func.type = type;
	stmt->func.identifier = identifier;
	stmt->func.params = params;
	stmt->func.is_function = is_function;
	return ETHER_SUCCESS;
}

static Stmt* parse_func_decl(void) {
	MAKE_STMT(new);
	error_code header_parsing_error = parse_func_header(new, false);
	if (header_parsing_error != ETHER_SUCCESS) return null;
	consume_right_bracket();

	new->type = STMT_FUNC;
	return new;
}

static void parse_load_stmt(void) {
	Token* fpath = null;
	expect_token_type(TOKEN_STRING, "expected string here: ");
	fpath = previous();
	consume_right_bracket();

	echar* cur_fpath = estr_create(srcfile->fpath);
	u64 last_forward_slash = estr_find_last_of(cur_fpath, '/'); /* TODO: change to '\' in windows */
	echar* target_fpath = null;
	target_fpath = estr_sub(cur_fpath, 0, last_forward_slash + 1);
	estr_append(target_fpath, fpath->lexeme);

	SourceFile* file = ether_read_file(target_fpath);
	if (!file) {
		error(fpath,
			  "cannot find \"%s\" (relative_to_working_dir: \"%s\");",
			  fpath->lexeme, target_fpath);
		return;
	}

	error_code err = loader_load(file, stmts);
	if (err) {
		/* TODO: what to do here */
	}
}

static Stmt* parse_var_decl(DataType* d, Token* t, bool is_global_var) {
	Expr* init = null;
	if (!match_token_type(TOKEN_RIGHT_BRACKET)) {
		/* TODO: in global variables, disable initializers, 
		 * or only constant (no variable reference) initializers  */
		init = parse_expr();
		consume_right_bracket();
	}

	MAKE_STMT(new);
	new->type = STMT_VAR_DECL;
	new->var_decl.type = d;
	new->var_decl.identifier = t;
	new->var_decl.initializer = init;
	new->var_decl.is_global_var = is_global_var;
	return new;
}

static Stmt* parse_if_stmt(void) {
	MAKE_STMT(new);
	new->type = STMT_IF;
	
	parse_if_branch(new, IF_IF_BRANCH);

	for (;;) {
		if (tokens[idx]->type == TOKEN_LEFT_BRACKET) {
			if ((idx+1) < tokens_len && tokens[idx+1]->type == TOKEN_KEYWORD) {
				if (str_intern(tokens[idx+1]->lexeme) ==
					str_intern("elif")) {
					goto_next_token();
					goto_next_token();
					parse_if_branch(new, IF_ELIF_BRANCH);
				} else break;
			} else break;
		} else break;
	}
	
	if (tokens[idx]->type == TOKEN_LEFT_BRACKET) {
		if ((idx+1) < tokens_len && tokens[idx+1]->type == TOKEN_KEYWORD) {
			if (str_intern(tokens[idx+1]->lexeme) ==
				str_intern("else")) {
				goto_next_token();
				goto_next_token();
				parse_if_branch(new, IF_ELSE_BRANCH);
			}
		}
	}

	return new;
}

static void parse_if_branch(Stmt* if_stmt, IfBranchType type) {
	Expr* cond = null;
	if (type != IF_ELSE_BRANCH) {
		cond = parse_expr();
	}

	Stmt** body = null;
	while (!match_right_bracket()) {
		CUR_ERROR;
		Stmt* stmt = parse_stmt();
		if (stmt) buf_push(body, stmt);
		EXIT_ERROR;
		CHECK_EOF_VOID_RETURN;
	}

	IfBranch* branch = (IfBranch*)malloc(sizeof(IfBranch));
	branch->cond = cond;
	branch->body = body;

	switch (type) {
		case IF_IF_BRANCH: {
			if_stmt->if_stmt.if_branch = branch;
		} break;

		case IF_ELIF_BRANCH: {
			buf_push(if_stmt->if_stmt.elif_branch, branch);
		} break;

		case IF_ELSE_BRANCH: {
			if_stmt->if_stmt.else_branch = branch;
		} break;
	}
}

static Stmt* parse_return_stmt(Token* keyword) {
	MAKE_STMT(new);
	Expr* expr = null;
	if (!match_right_bracket()) {
		expr = parse_expr();
		consume_right_bracket();
	}
	new->type = STMT_RETURN;
	new->return_stmt.expr = expr;
	new->return_stmt.keyword = keyword;
	assert(current_function);
	new->return_stmt.function_referernced = current_function;
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
	return parse_dot_access_expr();
}

static Expr* parse_dot_access_expr(void) {
	Expr* left = parse_primary_expr();
	while (match_token_type(TOKEN_DOT)) {
		Token* right = consume_identifier();
		left = make_dot_access_expr(left, right);
	}
	return left;
}

static Expr* parse_primary_expr(void) {
	if (match_token_type(TOKEN_NUMBER)) {
		return make_number_expr(previous());
	}
	else if (match_token_type(TOKEN_CHAR)) {
		return make_char_expr(previous());
	}
	else if (match_token_type(TOKEN_STRING)) {
		return make_string_expr(previous());
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
			bool got_valid_operator_keyword = false;
			for (u64 keyword = 0; keyword < buf_len(operator_keywords); ++keyword) {
				if (match_keyword(operator_keywords[keyword])) {
					callee = previous();
					got_valid_operator_keyword = true;
					break;
				}
			}

			if (!got_valid_operator_keyword) {
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

static Expr* make_dot_access_expr(Expr* left, Token* right) {
	MAKE_EXPR(new);
	new->type = EXPR_DOT_ACCESS;
	new->head = left->head;
	new->dot.left = left;
	new->dot.right = right;
	return new;
}

static Expr* make_number_expr(Token* t) {
	MAKE_EXPR(new);
	new->type = EXPR_NUMBER;
	new->head = t;
	new->number = t;
	return new;
}

static Expr* make_char_expr(Token* t) {
	MAKE_EXPR(new);
	new->type = EXPR_CHAR;
	new->head = t;
	new->chr = t;
	return new;
}

static Expr* make_string_expr(Token* t) {
	MAKE_EXPR(new);
	new->type = EXPR_STRING;
	new->head = t;
	new->string = t;
	return new;
}

static Expr* make_variable_expr(Token* t) {
	MAKE_EXPR(new);
	new->type = EXPR_VARIABLE;
	new->head = t;
	new->number = t;
	return new;
}

static Expr* make_func_call_expr(Token* callee, Expr** args) {
	MAKE_EXPR(new);
	new->type = EXPR_FUNC_CALL;
	new->head = callee;
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
	char buf[512];
	vsprintf(buf, msg, ap);
	printf("%s", buf);
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

static void init_built_in_data_types(void) {
	buf_push(built_in_data_types, str_intern("int"));
	buf_push(built_in_data_types, str_intern("char"));
	buf_push(built_in_data_types, str_intern("bool"));
	buf_push(built_in_data_types, str_intern("void"));
}

static void init_operator_keywords(void) {
	buf_push(operator_keywords, str_intern("set"));
	buf_push(operator_keywords, str_intern("deref"));
	buf_push(operator_keywords, str_intern("addr"));
	buf_push(operator_keywords, str_intern("at"));
}

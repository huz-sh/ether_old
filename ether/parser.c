#include <ether/ether.h>

static void parser_destroy(Parser*);

static Stmt* parse_decl(Parser*);
static Stmt* parse_stmt(Parser*);
static Stmt* parse_struct(Parser*, Token*);
static Stmt* parse_func(Parser*, bool);
static error_code parse_func_header(Parser*, Stmt*, bool);
static Stmt* parse_func_decl(Parser*);
static void parse_load_stmt(Parser*);
static Stmt* parse_var_decl(Parser*, DataType*, Token*, bool);
static Stmt* parse_if_stmt(Parser*);
static void parse_if_branch(Parser*, Stmt*, IfBranchType);
static Stmt* parse_for_stmt(Parser*);
static Stmt* parse_return_stmt(Parser*, Token*);
static Stmt* parse_expr_stmt(Parser*);

static Expr* parse_expr(Parser*);
static Expr* parse_dot_access_expr(Parser*);
static Expr* parse_primary_expr(Parser*);
static Expr* parse_func_call_expr(Parser*);

static Expr* make_dot_access_expr(Parser*, Expr*, Token*);
static Expr* make_number_expr(Parser*, Token*);
static Expr* make_char_expr(Parser*, Token*);
static Expr* make_string_expr(Parser*, Token*);
static Expr* make_variable_expr(Parser*, Token*);
static Expr* make_func_call_expr(Parser*, Token*, Expr**);

static bool match_token_type(Parser*, TokenType);
inline static bool match_left_bracket(Parser*);
inline static bool match_right_bracket(Parser*);
static bool match_keyword(Parser*, char*);
static DataType* match_data_type(Parser*);
static bool peek(Parser*, TokenType);
static void expect_token_type(Parser*, TokenType, const char*, ...);

inline static void consume_left_bracket(Parser*);
inline static void consume_right_bracket(Parser*);
inline static void consume_colon(Parser*);
inline static Token* consume_identifier(Parser*);
static DataType* consume_data_type(Parser*);

static void goto_next_token(Parser*);
static void goto_previous_token(Parser*);
static Token* current(Parser*);
static Token* previous(Parser*);

static void error_at_current(Parser*, const char*, ...);
static void error(Parser*, Token*, const char*, ...);
static void warning_at_previous(Parser*, const char*, ...);
static void warning(Parser*, Token*, const char*, ...);
static void sync_to_next_statement(Parser*);

static void init_built_in_data_types(Parser*);
static void init_operator_keywords(Parser*);

#define CUR_ERROR uint current_error_count = p->error_count
#define EXIT_ERROR if (p->error_count > current_error_count) return

#define CHECK_EOF(x) \
if (current(p)->type == TOKEN_EOF) { \
	error_at_current(p, "end of file while parsing function body; did you forget a ']'?"); \
	return (x); \
}

#define CHECK_EOF_VOID_RETURN \
if (current(p)->type == TOKEN_EOF) { \
	error_at_current(p, "end of file while parsing function body; did you forget a ']'?"); \
	return; \
}

Stmt** parser_run(Parser* p, Token** tokens_buf, SourceFile* file, error_code* out_error_code) {
	p->tokens = tokens_buf;
	p->tokens_len = buf_len(p->tokens);
	p->stmts = null;
	p->srcfile = file;
	p->built_in_data_types = null;
	p->operator_keywords = null;
	p->idx = 0;
	p->error_count = 0;
	p->error_occured = false;
	p->error_bracket_counter = 0;
	p->error_panic = false;
	p->start_stmt_bracket = false;
	p->current_function = null;

	init_built_in_data_types(p);
	init_operator_keywords(p);

	while (current(p)->type != TOKEN_EOF) {
		Stmt* stmt = parse_decl(p);
		if (stmt) buf_push(p->stmts, stmt);
	}
	if (out_error_code) *out_error_code = p->error_occured;
	parser_destroy(p);
	return p->stmts;
}

void parser_destroy(Parser* p) {
	buf_free(p->built_in_data_types);
}

/* only top level statements (functions, structs, var) */
static Stmt* parse_decl(Parser* p) {
	consume_left_bracket(p);
 
	/* TODO: parser necessary data types and identifiers in functions
	 * and leave this clean */
	Stmt* stmt = null;
	if (match_keyword(p, "struct")) {
		Token* identifier = consume_identifier(p); /* TODO: to refactor */
		stmt = parse_struct(p, identifier);
	}
	else if (match_keyword(p, "let")) {
		DataType* type = consume_data_type(p); /* TODO: to refactor */
		consume_colon(p);
		Token* identifier = consume_identifier(p);
		stmt = parse_var_decl(p, type, identifier, true);
	}
	else if (match_keyword(p, "defn")) {
		bool public = false;
		if (match_keyword(p, "pub")) public = true;
		stmt = parse_func(p, public);
	}
	else if (match_keyword(p, "decl")) {
		stmt = parse_func_decl(p);
	}
	else if (match_keyword(p, "load")) {
		parse_load_stmt(p);
		return null;
	}
	else {
		error_at_current(p, "expected keyword 'load', 'struct', 'defn', 'decl' or 'let' "
						 "in global scope; did you miss a ']'?");
		return null;
	}

	return stmt;
}

/* only statements inside scope / function */
static Stmt* parse_stmt(Parser* p) {
	p->start_stmt_bracket = true;
	consume_left_bracket(p);
	p->start_stmt_bracket = false;

	Stmt* stmt = null;
	if (match_keyword(p, "let")) {
		DataType* dt = consume_data_type(p);
		consume_colon(p);
		Token* identifier = consume_identifier(p);
		stmt = parse_var_decl(p, dt, identifier, false);
	}
	else if (match_keyword(p, "return")) {
		stmt = parse_return_stmt(p, previous(p));
	}
	else if (match_keyword(p, "if")) {
		stmt = parse_if_stmt(p);
	}
	else if (match_keyword(p, "for")) {
		stmt = parse_for_stmt(p);
	}
	else if (match_keyword(p, "elif") || match_keyword(p, "else")) {
		error(p, previous(p), "'%s' branch without preceding 'if' statement; "
						  "did you mean 'if'?", previous(p)->lexeme);
		return null;
	}
	else if (match_keyword(p, "struct")) {
		error(p, previous(p), "cannot define a type inside a function-scope; "
						  "did you miss a ']'?");
		return null;
	}
	else if (match_keyword(p, "defn")) {
		error(p, previous(p), "cannot define a function inside a function-scope; "
						  "did you miss a ']'?");
		return null;
	}
	else if (match_keyword(p, "decl")) {
		error(p, previous(p), "cannot declare a function inside a function-scope; "
						  "did you miss a ']'?");
		return null;
	}
	else {
		goto_previous_token(p);
		stmt = parse_expr_stmt(p);
	}

	p->error_panic = false;
	return stmt;
}

#define MAKE_STMT(x) Stmt* x = (Stmt*)malloc(sizeof(Stmt));

static Stmt* parse_struct(Parser* p, Token* identifier) {
	Field** fields = null;
	if (!match_right_bracket(p)) {
		do {
			consume_left_bracket(p);
			if (!match_keyword(p, "let")) {
				/* TODO: refactor into function */
				error(p, current(p), "expected 'let' keyword here: ");
				continue;
			}
			DataType* d = consume_data_type(p);
			consume_colon(p);
			Token* t = consume_identifier(p);

			Field* f = (Field*)malloc(sizeof(Field));
			f->type = d;
			f->identifier = t;
			buf_push(fields, f);
			consume_right_bracket(p);
			CHECK_EOF(null);
		} while (!match_right_bracket(p));
	}

	MAKE_STMT(new);
	new->type = STMT_STRUCT;
	new->struct_stmt.identifier = identifier;
	new->struct_stmt.fields = fields;
	return new;
}

static Stmt* parse_func(Parser* p, bool public) {
	MAKE_STMT(new);
	error_code header_parsing_error = parse_func_header(p, new, true);
	if (header_parsing_error != ETHER_SUCCESS) return null;
	p->current_function = new;

	Stmt** body = null;
	while (!match_right_bracket(p)) {
		CUR_ERROR;
		CHECK_EOF(null);
		Stmt* stmt = parse_stmt(p);
		if (stmt) buf_push(body, stmt);
		EXIT_ERROR null;
	}

	new->type = STMT_FUNC;
	new->func.body = body;
	new->func.public = public;
	return new;
}

static error_code parse_func_header(Parser* p, Stmt* stmt, bool is_function) {
	DataType* type = consume_data_type(p);
	consume_colon(p);
	Token* identifier = consume_identifier(p);
	consume_left_bracket(p);

	Stmt** params = null;
	bool has_params = true;

	if (current(p)->type == TOKEN_KEYWORD) {
		if (str_intern(current(p)->lexeme) ==
			str_intern("void")) {

			if (p->idx < p->tokens_len-1 && p->tokens[p->idx+1]->type == TOKEN_RIGHT_BRACKET) {
				has_params = false;
			}
		}
	}

	if (!has_params) {
		goto_next_token(p);
		consume_right_bracket(p);
	}
	else if (peek(p, TOKEN_RIGHT_BRACKET)) {
		/* we call warning at 'previous' because we want to
		 * mark the left bracket as the cause */
		warning_at_previous(p, "empty function parameter list here:");
		goto_next_token(p);
	}
	else {
		do {
			CUR_ERROR;
			DataType* p_type = consume_data_type(p);
			consume_colon(p);
			Token* p_name = consume_identifier(p);

			MAKE_STMT(param);
			param->type = STMT_VAR_DECL;
			param->var_decl.type = p_type;
			param->var_decl.identifier = p_name;
			param->var_decl.initializer = null;
			param->var_decl.is_global_var = false;
			buf_push(params, param);

			EXIT_ERROR ETHER_ERROR;
			CHECK_EOF(ETHER_ERROR);
		} while (!match_right_bracket(p));
	}

	stmt->func.type = type;
	stmt->func.identifier = identifier;
	stmt->func.params = params;
	stmt->func.is_function = is_function;
	return ETHER_SUCCESS;
}

static Stmt* parse_func_decl(Parser* p) {
	MAKE_STMT(new);
	error_code header_parsing_error = parse_func_header(p, new, false);
	if (header_parsing_error != ETHER_SUCCESS) return null;
	consume_right_bracket(p);

	new->type = STMT_FUNC;
	new->func.public = true;
	return new;
}

static void parse_load_stmt(Parser* p) {
	Token* fpath = null;
	expect_token_type(p, TOKEN_STRING, "expected string here: ");
	fpath = previous(p);
	consume_right_bracket(p);

	echar* cur_fpath = estr_create(p->srcfile->fpath);
	u64 last_forward_slash = estr_find_last_of(cur_fpath, '/'); /* TODO: change to '\' in windows */
	echar* target_fpath = null;
	target_fpath = estr_sub(cur_fpath, 0, last_forward_slash + 1);
	estr_append(target_fpath, fpath->lexeme);

	SourceFile* file = ether_read_file(target_fpath);
	if (!file) {
		error(p, fpath,
			  "cannot find \"%s\" (relative_to_working_dir: \"%s\");",
			  fpath->lexeme, target_fpath);
		return;
	}

	// Loader loader;
	// error_code err = loader_load(&loader, file, p->stmts);
	// if (err) {
		/* TODO: what to do here */
	// }
}

static Stmt* parse_var_decl(Parser* p, DataType* d, Token* t, bool is_global_var) {
	Expr* init = null;
	if (!match_token_type(p, TOKEN_RIGHT_BRACKET)) {
		/* TODO: in global variables, disable initializers,
		 * or only constant (no variable reference) initializers  */
		init = parse_expr(p);
		consume_right_bracket(p);
	}

	MAKE_STMT(new);
	new->type = STMT_VAR_DECL;
	new->var_decl.type = d;
	new->var_decl.identifier = t;
	new->var_decl.initializer = init;
	new->var_decl.is_global_var = is_global_var;
	return new;
}

static Stmt* parse_if_stmt(Parser* p) {
	MAKE_STMT(new);
	new->type = STMT_IF;

	parse_if_branch(p, new, IF_IF_BRANCH);

	for (;;) {
		if (p->tokens[p->idx]->type == TOKEN_LEFT_BRACKET) {
			if ((p->idx+1) < p->tokens_len && p->tokens[p->idx+1]->type == TOKEN_KEYWORD) {
				if (str_intern(p->tokens[p->idx+1]->lexeme) ==
					str_intern("elif")) {
					goto_next_token(p);
					goto_next_token(p);
					parse_if_branch(p, new, IF_ELIF_BRANCH);
				} else break;
			} else break;
		} else break;
	}

	if (p->tokens[p->idx]->type == TOKEN_LEFT_BRACKET) {
		if ((p->idx+1) < p->tokens_len && p->tokens[p->idx+1]->type == TOKEN_KEYWORD) {
			if (str_intern(p->tokens[p->idx+1]->lexeme) ==
				str_intern("else")) {
				goto_next_token(p);
				goto_next_token(p);
				parse_if_branch(p, new, IF_ELSE_BRANCH);
			}
		}
	}

	return new;
}

static void parse_if_branch(Parser* p, Stmt* if_stmt, IfBranchType type) {
	Expr* cond = null;
	if (type != IF_ELSE_BRANCH) {
		cond = parse_expr(p);
	}

	Stmt** body = null;
	while (!match_right_bracket(p)) {
		CUR_ERROR;
		Stmt* stmt = parse_stmt(p);
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

static Stmt* parse_for_stmt(Parser* p) {
	Token* identifier = consume_identifier(p);
	if (!match_keyword(p, "to")) {
		error(p, current(p), "expected 'to' keyword here: ");
		return null;
	}

	Expr* to = parse_expr(p);

	Stmt** body = null;
	while (!match_right_bracket(p)) {
		CUR_ERROR;
		Stmt* stmt = parse_stmt(p);
		if (stmt) buf_push(body, stmt);
		EXIT_ERROR null;
		CHECK_EOF(null);
	}
	
	MAKE_STMT(counter);
	counter->type = STMT_VAR_DECL;
	counter->var_decl.type = null;
	counter->var_decl.identifier = identifier;
	counter->var_decl.initializer = null;
	counter->var_decl.is_global_var = false;

	MAKE_STMT(new);
	new->type = STMT_FOR;
	new->for_stmt.counter = counter;
	new->for_stmt.to = to;
	new->for_stmt.body = body;
	return new;
}

static Stmt* parse_return_stmt(Parser* p, Token* keyword) {
	MAKE_STMT(new);
	Expr* expr = null;
	if (!match_right_bracket(p)) {
		expr = parse_expr(p);
		consume_right_bracket(p);
	}
	new->type = STMT_RETURN;
	new->return_stmt.expr = expr;
	new->return_stmt.keyword = keyword;
	assert(p->current_function);
	new->return_stmt.function_referernced = p->current_function;
	return new;
}

static Stmt* parse_expr_stmt(Parser* p) {
	MAKE_STMT(new);
	Expr* e = parse_expr(p);
	new->type = STMT_EXPR;
	new->expr = e;
	return new;
}

static Expr* parse_expr(Parser* p) {
	return parse_dot_access_expr(p);
}

static Expr* parse_dot_access_expr(Parser* p) {
	Expr* left = parse_primary_expr(p);
	while (match_token_type(p, TOKEN_DOT)) {
		Token* right = consume_identifier(p);
		left = make_dot_access_expr(p, left, right);
	}
	return left;
}

static Expr* parse_primary_expr(Parser* p) {
	if (match_token_type(p, TOKEN_NUMBER)) {
		return make_number_expr(p, previous(p));
	}
	else if (match_token_type(p, TOKEN_CHAR)) {
		return make_char_expr(p, previous(p));
	}
	else if (match_token_type(p, TOKEN_STRING)) {
		return make_string_expr(p, previous(p));
	}
	else if (match_token_type(p, TOKEN_IDENTIFIER)) {
		return make_variable_expr(p, previous(p));
	}
	else if (match_left_bracket(p)) {
		return parse_func_call_expr(p);
	}
	else {
		error_at_current(p, "invalid syntax; expected identifier, "
		 				 "literal, or grouping but got '%s'",
						 current(p)->lexeme);
	}
	return null;
}

static Expr* parse_func_call_expr(Parser* p) {
	Token* callee = null;
	switch (current(p)->type) {
		case TOKEN_IDENTIFIER:
		case TOKEN_PLUS:
		case TOKEN_MINUS:
		case TOKEN_STAR:
		case TOKEN_SLASH:
		case TOKEN_PERCENT:
		case TOKEN_EQUAL:
		case TOKEN_LESS:
		case TOKEN_LESS_EQUAL:
		case TOKEN_GREATER:
		case TOKEN_GREATER_EQUAL: {
			callee = current(p);
			goto_next_token(p);
		} break;

		default: {
			bool got_valid_operator_keyword = false;
			for (u64 keyword = 0; keyword < buf_len(p->operator_keywords); ++keyword) {
				if (match_keyword(p, p->operator_keywords[keyword])) {
					callee = previous(p);
					got_valid_operator_keyword = true;
					break;
				}
			}

			if (!got_valid_operator_keyword) {
				error_at_current(p, "expected identifier or operator here:");
				return null;
			}
		}
	}

	Expr** args = null;
	if (!match_right_bracket(p)) {
		do {
			CUR_ERROR;
			Expr* e = parse_expr(p);
			if (e) buf_push(args, e);
			CHECK_EOF(null);
			EXIT_ERROR null;
		} while (!match_right_bracket(p));
	}

	return make_func_call_expr(p, callee, args);
}

#define MAKE_EXPR(x) Expr* x = (Expr*)malloc(sizeof(Expr));

static Expr* make_dot_access_expr(Parser* p, Expr* left, Token* right) {
	MAKE_EXPR(new);
	new->type = EXPR_DOT_ACCESS;
	new->head = left->head;
	new->dot.left = left;
	new->dot.right = right;
	return new;
}

static Expr* make_number_expr(Parser* p, Token* t) {
	MAKE_EXPR(new);
	new->type = EXPR_NUMBER;
	new->head = t;
	new->number = t;
	return new;
}

static Expr* make_char_expr(Parser* p, Token* t) {
	MAKE_EXPR(new);
	new->type = EXPR_CHAR;
	new->head = t;
	new->chr = t;
	return new;
}

static Expr* make_string_expr(Parser* p, Token* t) {
	MAKE_EXPR(new);
	new->type = EXPR_STRING;
	new->head = t;
	new->string = t;
	return new;
}

static Expr* make_variable_expr(Parser* p, Token* t) {
	MAKE_EXPR(new);
	new->type = EXPR_VARIABLE;
	new->head = t;
	new->number = t;
	return new;
}

static Expr* make_func_call_expr(Parser* p, Token* callee, Expr** args) {
	MAKE_EXPR(new);
	new->type = EXPR_FUNC_CALL;
	new->head = callee;
	new->func_call.callee = callee;
	new->func_call.args = args;
	return new;
}

static bool match_token_type(Parser* p, TokenType t) {
	if (peek(p, t)) {
		goto_next_token(p);
		return true;
	}
	return false;
}

static bool match_left_bracket(Parser* p) {
	if (match_token_type(p, TOKEN_LEFT_BRACKET)) {
		return true;
	}
	return false;
}

static bool match_right_bracket(Parser* p) {
	if (match_token_type(p, TOKEN_RIGHT_BRACKET)) {
		return true;
	}
	return false;
}

static bool match_keyword(Parser* p, char* s) {
	if (peek(p, TOKEN_KEYWORD) &&
		str_intern(current(p)->lexeme) == str_intern(s)) {
		goto_next_token(p);
		return true;
	}
	return false;
}

static DataType* match_data_type(Parser* p) {
	DataType* new = null;
	bool matched_main_type = false;
	if (match_token_type(p, TOKEN_IDENTIFIER)) {
		matched_main_type = true;
		new = (DataType*)malloc(sizeof(DataType));
		new->type = previous(p);
	}
	else {
		for (uint i = 0; i < buf_len(p->built_in_data_types); ++i) {
			if (match_keyword(p, p->built_in_data_types[i])) {
				matched_main_type = true;
				new = (DataType*)malloc(sizeof(DataType));
				new->type = previous(p);
			}
		}
	}

	if (matched_main_type) {
		u8 pointer_count = 0;
		while (match_token_type(p, TOKEN_STAR)) {
			pointer_count++;
		}
		new->pointer_count = pointer_count;
	}

	return new;
}

static bool peek(Parser* p, TokenType t) {
	if (p->idx >= p->tokens_len) {
		return false;
	}

	if (current(p)->type == t) {
		return true;
	}
	return false;
}

static void expect_token_type(Parser* p, TokenType t, const char* msg, ...) {
	if (match_token_type(p, t)) {
		return;
	}
	va_list ap;
	va_start(ap, msg);
	error_at_current(p, msg, ap);
	va_end(ap);
}

inline static void consume_left_bracket(Parser* p) {
	if (!p->start_stmt_bracket) p->error_bracket_counter++;
	expect_token_type(p, TOKEN_LEFT_BRACKET, "expected '[' here:");
}

inline static void consume_right_bracket(Parser* p) {
	expect_token_type(p, TOKEN_RIGHT_BRACKET, "expected ']' here:");
}

inline static void consume_colon(Parser* p) {
	expect_token_type(p, TOKEN_COLON, "expected ':' here:");
}

inline static Token* consume_identifier(Parser* p) {
	expect_token_type(p, TOKEN_IDENTIFIER, "expected identifier here:");
	return previous(p);
}

static DataType* consume_data_type(Parser* p) {
	DataType* type = match_data_type(p);
	if (!type) {
		error_at_current(p, "expected data type here:");
	}
	return type;
}

static void goto_next_token(Parser* p) {
	if (p->idx == 0 || (p->idx - 1) < p->tokens_len) {
		++p->idx;
	}
}

static void goto_previous_token(Parser* p) {
	if (p->idx > 0) {
		--p->idx;
	}
}

static Token* current(Parser* p) {
	if (p->idx >= p->tokens_len) {
		return null;
	}
	return p->tokens[p->idx];
}

static Token* previous(Parser* p) {
	if (p->idx >= p->tokens_len + 1) {
		return null;
	}
	return p->tokens[p->idx - 1];
}

static void error_at_current(Parser* p, const char* msg, ...) {
	va_list ap;
	va_start(ap, msg);
	error(p, current(p), msg, ap);
	va_end(ap);
}

static void error(Parser* p, Token* t, const char* msg, ...) {
	if (p->error_panic) return;
	p->error_panic = true;

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

	sync_to_next_statement(p);

	p->error_occured = ETHER_ERROR;
	++p->error_count;
}

static void warning_at_previous(Parser* p, const char* msg, ...) {
	va_list ap;
	va_start(ap, msg);
	warning(p, previous(p), msg, ap);
	va_end(ap);
}

static void warning(Parser* p, Token* t, const char* msg, ...) {
	va_list ap;
	va_start(ap, msg);
	printf("%s:%ld:%d: warning: ", t->srcfile->fpath, t->line, t->column);
	vprintf(msg, ap);
	va_end(ap);
	printf("\n");

	print_file_line_with_info(t->srcfile, t->line);
	print_marker_arrow_with_info_ln(t->srcfile, t->line, t->column);
}

static void sync_to_next_statement(Parser* p) {
	while (true) {
		if (current(p)->type == TOKEN_RIGHT_BRACKET) {
			if (p->error_bracket_counter == 0) {
				/* stmt bkt */
				goto_next_token(p);
				return;
			}
			else if (p->error_bracket_counter > 0) {
				p->error_bracket_counter--;
				goto_next_token(p);
			}
		}
		else if (current(p)->type == TOKEN_LEFT_BRACKET) {
			p->error_bracket_counter++;
			goto_next_token(p);
		}
		else {
			if (current(p)->type == TOKEN_EOF) {
				++p->error_count;
				return;
			}
			goto_next_token(p);
		}
	}
}

static void init_built_in_data_types(Parser* p) {
	buf_push(p->built_in_data_types, str_intern("int"));

	buf_push(p->built_in_data_types, str_intern("i8"));
	buf_push(p->built_in_data_types, str_intern("i16"));
	buf_push(p->built_in_data_types, str_intern("132"));
	buf_push(p->built_in_data_types, str_intern("i64"));
	
	buf_push(p->built_in_data_types, str_intern("u8"));
	buf_push(p->built_in_data_types, str_intern("u16"));
	buf_push(p->built_in_data_types, str_intern("u32"));
	buf_push(p->built_in_data_types, str_intern("u64"));

	buf_push(p->built_in_data_types, str_intern("char"));
	buf_push(p->built_in_data_types, str_intern("bool"));
	buf_push(p->built_in_data_types, str_intern("void"));
}

static void init_operator_keywords(Parser* p) {
	buf_push(p->operator_keywords, str_intern("set"));
	buf_push(p->operator_keywords, str_intern("deref"));
	buf_push(p->operator_keywords, str_intern("addr"));
	buf_push(p->operator_keywords, str_intern("at"));
}

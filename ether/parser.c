#include <ether/ether.h>

/**** PARSER STATE VARIABLES ****/
static file srcfile;
static token** tokens;
static uint64 tokens_len;
static stmt** stmts;

static uint64 idx;
static uint error_count;
static int error_occured;
static int error_bkt_counter;
static int error_panic;
static int start_stmt_bkt;

static char** built_in_data_types;

/**** PARSER FUNCTIONS ****/
static stmt* decl(void);
static stmt* _stmt(void);
static stmt* _struct(token*);
static stmt* func(data_type*, token*);
static stmt* var_decl(data_type*, token*);
static stmt* expr_stmt(void);

static expr* _expr(void);
static expr* expr_primary(void);
static expr* expr_func_call(void);

static expr* make_number_expr(token*);
static expr* make_variable_expr(token*);
static expr* make_func_call(token*, expr**);

static int match(token_type);
static int match_keyword(char*);
static data_type* match_data_type(void);
static int peek(token_type t); 
static void expect(token_type, const char*, ...);

inline static void consume_lbkt(void);
inline static void consume_rbkt(void);
inline static void consume_colon(void);
inline static token* consume_identifier(void);
static data_type* consume_data_type(void);

static void goto_next_tok(void);
static void goto_prev_tok(void);
static token* cur(void);
static token* prev(void);

static void errorc(const char*, ...);
static void errorp(const char*, ...);
static void error(token*, const char*, ...);
static void error_sync(void);

#define CUR_ERROR uint err_count = error_count
#define EXIT_ERROR if (error_count > err_count) return

#define CHECK_EOF(x) \
if (cur()->type == TOKEN_EOF) { \
	errorc("end of file while parsing function body; did you forget a ']'?"); \
	return (x);															\
}

void parser_init(file src, token** t) {
	srcfile = src;
	tokens = t;
	tokens_len = buf_len(t);
	idx = 0UL;
	error_count = 0;
	error_occured = false;
	error_bkt_counter = 0;
	error_panic = false;
	start_stmt_bkt = false;

	buf_push(built_in_data_types, stri("int"));
	buf_push(built_in_data_types, stri("char"));
	buf_push(built_in_data_types, stri("void"));
}

stmt** parser_run(int* err) {
	while (cur()->type != TOKEN_EOF) {
		stmt* s = decl();
		if (s) buf_push(stmts, s);
	}
	if (err) *err = error_occured;
	return stmts;
}

/* only top level statements (functions, structs, var) */
static stmt* decl(void) {
	consume_lbkt();

	stmt* s = null;
	if (match_keyword(stri("struct"))) {
		token* identifier = consume_identifier();
		s = _struct(identifier);
	}
	else if (match_keyword(stri("let"))) { 
		data_type* type = consume_data_type();
		consume_colon();
		token* identifier = consume_identifier();
		s = var_decl(type, identifier);
		consume_rbkt();
	}
	else {
		data_type* type = consume_data_type();
		consume_colon();
		token* identifier = consume_identifier();
		consume_lbkt();
		s = func(type, identifier);
	}
	
	return s;
}

/* only statements inside scope / function */
static stmt* _stmt(void) {
	start_stmt_bkt = true;
	consume_lbkt();
	start_stmt_bkt = false;
	
	stmt* s = null;
	if (match_keyword("let")) {
		data_type* dt = consume_data_type();
		consume_colon();
		token* identifier = consume_identifier();
		s = var_decl(dt, identifier);
		consume_rbkt();
	}
	else {
		goto_prev_tok();
		s = expr_stmt();
	}
	
	error_panic = false;
	return s;
}

#define MAKE_STMT(x) stmt* x = (stmt*)malloc(sizeof(stmt));

static stmt* _struct(token* identifier) {
	struct_field** fields = null;
	if (!match(TOKEN_R_BKT)) {
		do {
			consume_lbkt();
			data_type* d = consume_data_type();
			consume_colon();
			token* t = consume_identifier();
			
			struct_field* f = (struct_field*)malloc(sizeof(struct_field));
			f->type = d;
			f->identifier = t;
			buf_push(fields, f);
			consume_rbkt();
			CHECK_EOF(null);
		} while (!match(TOKEN_R_BKT));
	}

	MAKE_STMT(new);
	new->type = STMT_STRUCT;
	new->_struct.identifier = identifier;
	new->_struct.fields = fields;
	return new;
}

static stmt* func(data_type* d, token* t) {
	param* params = null;
	if (match_keyword("void")) {
		consume_rbkt();
	}
	else if (!match(TOKEN_R_BKT)) {
		do {
			CUR_ERROR;
			data_type* p_type = consume_data_type();
			consume_colon();
			token* p_name = consume_identifier();
			buf_push(params, (param){ p_type, p_name });
			EXIT_ERROR null;
			CHECK_EOF(null);
		} while (!match(TOKEN_R_BKT));
	}

	stmt** body = null;
	while (!match(TOKEN_R_BKT)) {
		CUR_ERROR;
		CHECK_EOF(null);
		stmt* s = _stmt();
		if (s) buf_push(body, s);
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

static stmt* var_decl(data_type* d, token* t) {
	expr* init = null;
	if (match(TOKEN_EQUAL)) {
		init = _expr();
	}
	
	MAKE_STMT(new);
	new->type = STMT_VAR_DECL;
	new->var_decl.type = d;
	new->var_decl.identifier = t;
	new->var_decl.initializer = init;
	return new;
}

static stmt* expr_stmt(void) {
	MAKE_STMT(new);
	expr* e = _expr();
	new->type = STMT_EXPR;
	new->expr_stmt = e;
	return new;
}

static expr* _expr(void) {
	return expr_primary();
}

static expr* expr_primary(void) {
	if (match(TOKEN_NUMBER)) {
		return make_number_expr(prev());
	}
	else if (match(TOKEN_IDENTIFIER)) {
		return make_variable_expr(prev());
	}
	else if (match(TOKEN_L_BKT)) {
		return expr_func_call();
	}
	else {
		errorc("invalid syntax; expected identifier, literal, or grouping but got '%s'", cur()->lexeme);
	}
	return null;
}

static expr* expr_func_call(void) {
	token* callee = null;
	switch (cur()->type) {
		case TOKEN_IDENTIFIER:
		case TOKEN_PLUS:
		case TOKEN_MINUS:
		case TOKEN_STAR:
		case TOKEN_SLASH:
		case TOKEN_EQUAL: {
			callee = cur();
			goto_next_tok();
		} break;

		default: {
			errorc("expected identifier or operator here:");
			return null;
		}
	}
	
	expr** args = null;
	if (!match(TOKEN_R_BKT)) {
		do {
			expr* e = _expr();
			if (e) buf_push(args, e);
		} while (!match(TOKEN_R_BKT));
	}

	return make_func_call(callee, args);
}

#define MAKE_EXPR(x) expr* x = (expr*)malloc(sizeof(expr));

static expr* make_number_expr(token* t) {
	MAKE_EXPR(new);
	new->type = EXPR_NUMBER;
	new->number = t;
	return new;
}

static expr* make_variable_expr(token* t) {
	MAKE_EXPR(new);
	new->type = EXPR_VARIABLE;
	new->number = t;
	return new;
}

static expr* make_func_call(token* callee, expr** args) {
	MAKE_EXPR(new);
	new->type = EXPR_FUNC_CALL;
	new->func_call.callee = callee;
	new->func_call.args = args;
	return new;
}

static int match(token_type t) {
	if (peek(t)) {
		goto_next_tok();
		return true;
	}
	return false;
}

static int match_keyword(char* s) {
	if (peek(TOKEN_KEYWORD) && stri(cur()->lexeme) == stri(s)) {
		goto_next_tok();
		return true;		
	}
	return false;
}

static int peek(token_type t) {
	if (idx >= tokens_len) {
		return false;
	}

	if (cur()->type == t) {
		return true;
	}	
	return false;
}

static data_type* match_data_type(void) {
	if (match(TOKEN_IDENTIFIER)) {
		data_type* new = (data_type*)malloc(sizeof(data_type));
		new->type = prev();
		return new;
	}
	for (uint i = 0; i < buf_len(built_in_data_types); ++i) {
		if (match_keyword(stri(built_in_data_types[i]))) {
			data_type* new = (data_type*)malloc(sizeof(data_type));
			new->type = prev();
			return new;			
		}
	}
	return null;
}

static void expect(token_type t, const char* msg, ...) {
	if (match(t)) {
		return;
	}

	va_list ap;
	va_start(ap, msg);
	errorc(msg, ap);
	va_end(ap);
}

inline static void consume_lbkt(void) {
	if (!start_stmt_bkt) error_bkt_counter++;
	expect(TOKEN_L_BKT, "expected '[' here:");
}

inline static void consume_rbkt(void) {
	expect(TOKEN_R_BKT, "expected ']' here:");
}

inline static void consume_colon(void) {
	expect(TOKEN_COLON, "expected ':' here:");
}

inline static token* consume_identifier(void) {
	expect(TOKEN_IDENTIFIER, "expected identifier here:");
	return prev();
}

static data_type* consume_data_type(void) {
	data_type* type = match_data_type();
	if (!type) {
		errorc("expected data type here:");
	}
	return type;
	
}

static void goto_next_tok(void) {
	if (idx == 0 || (idx - 1) < tokens_len) {
		++idx;
	}
}

static void goto_prev_tok(void) {
	if (idx > 0) {
		--idx;
	}
}

static token* cur(void) {
	if (idx >= tokens_len) {
		return null;
	}
	return tokens[idx];		
}

static token* prev(void) {
	if (idx >= tokens_len + 1) {
		return null;
	}
	return tokens[idx - 1];
}

static void errorc(const char* msg, ...) {
	va_list ap;
	va_start(ap, msg);
	error(cur(), msg, ap);
	va_end(ap);	
}

/* TODO: check if we need errorp in the final build */
static void errorp(const char* msg, ...) {
	va_list ap;
	va_start(ap, msg);
	error(prev(), msg, ap);
	va_end(ap);
}

static void error(token* t, const char* msg, ...) {	
	if (error_panic) return;
	error_panic = true;
	
	va_list ap;
	va_start(ap, msg);
	printf("%s:%ld:%d: error: ", srcfile.fpath, t->line, t->col);
	vprintf(msg, ap);
	va_end(ap);
	printf("\n");

	print_file_line_with_info(srcfile, t->line);
	print_marker_arrow_with_info_ln(srcfile, t->line, t->col);

	error_sync();
	
	error_occured = ETHER_ERROR;
	++error_count;
}

static void error_sync(void) {
	while (true) {
		if (cur()->type == TOKEN_R_BKT) {
			if (error_bkt_counter == 0) {
				/* stmt bkt */
				goto_next_tok();
				return;
			}
			else if (error_bkt_counter > 0) {
				error_bkt_counter--;
				goto_next_tok();
			}
		}
		else if (cur()->type == TOKEN_L_BKT) {
			error_bkt_counter++;
			goto_next_tok();
		}
		else {
			if (cur()->type == TOKEN_EOF) {
				++error_count;
				return;
			}
			goto_next_tok();
		}
	}
}

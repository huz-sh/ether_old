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

/**** PARSER FUNCTIONS ****/
static stmt* decl(void);
static stmt* _stmt(void);
static stmt* var_decl(data_type*, token*);
static stmt* expr_stmt(void);

static expr* _expr(void);
static expr* _expr_no_bkt(void);
static expr* expr_assign(void);
static expr* expr_add_sub(void);
static expr* expr_mul_div(void);
static expr* expr_primary(void);

static expr* make_binary_expr(expr*, expr*, token*);
static expr* make_number_expr(token*);
static expr* make_variable_expr(token*);

static int match(token_type);
static data_type* match_data_type(void);
static void expect(token_type, const char*, ...);

inline static void consume_lbkt(void);
inline static void consume_rbkt(void);
inline static void consume_colon(void);
inline static token* consume_identifier(void);
static data_type* consume_data_type(void);

static void goto_next_tok(void);
static token* cur(void);
static token* prev(void);

static void errorc(const char*, ...);
static void errorp(const char*, ...);
static void error(token*, const char*, ...);
static void error_sync(void);

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
}

stmt** parser_run(int* err) {
	while (cur()->type != TOKEN_EOF) {
		stmt* s = _stmt(); /* TODO: add decl instead of stmt */
		if (s) buf_push(stmts, s);
	}
	if (err) *err = error_occured;
	return stmts;
}

/* only top level statements (functions, structs, var) */
static stmt* decl(void) {
	consume_lbkt();
	
	data_type* type = consume_data_type();
	consume_colon();
	token* identifier = consume_identifier();
	if (!match(TOKEN_L_BKT)) {
		/* variable declaration */
		stmt* s = var_decl(type, identifier);
		consume_rbkt();
		return s;
	}
	return null;
}

/* only statements inside scope / function */
static stmt* _stmt(void) {
	start_stmt_bkt = true;
	consume_lbkt();
	start_stmt_bkt = false;
	
	stmt* s = null;
	data_type* dt = null;
	if ((dt = match_data_type()) != null) {
		consume_colon();
		token* identifier = consume_identifier();
		s = var_decl(dt, identifier);
	}
	else {
		s = expr_stmt();
	}
	
	consume_rbkt();
	error_panic = false;
	return s;
}

#define MAKE_STMT(x) stmt* x = (stmt*)malloc(sizeof(stmt));

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
	expr* e = _expr_no_bkt();
	new->type = STMT_EXPR;
	new->expr_stmt = e;
	return new;
}

static expr* _expr(void) {
	consume_lbkt();
	expr* e = expr_assign();
	consume_rbkt();
	return e;
}

static expr* _expr_no_bkt(void) {
	return expr_assign();
}

static expr* expr_assign(void) {
	expr* left = expr_add_sub();
	if (match(TOKEN_EQUAL)) {
		token* eq = prev();
		expr* right = expr_assign();
		return make_binary_expr(left, right, eq);
	}
	return left;
}

static expr* expr_add_sub(void) {
	expr* left = expr_mul_div();
	while (match(TOKEN_PLUS) || match(TOKEN_MINUS)) {
		token* op = prev();
		expr* right = expr_mul_div();
		left = make_binary_expr(left, right, op);
	}
	return left;
}

static expr* expr_mul_div(void) {
	expr* left = expr_primary();
	while (match(TOKEN_STAR) || match(TOKEN_SLASH)) {
		token* op = prev();
		expr* right = expr_primary();
		left = make_binary_expr(left, right, op);
	}
	return left;
}

static expr* expr_primary(void) {
	if (match(TOKEN_NUMBER)) {
		return make_number_expr(prev());
	}
	else if (match(TOKEN_IDENTIFIER)) {
		return make_variable_expr(prev());
	}
	else if (match(TOKEN_L_BKT)) {
		expr* e = expr_assign();
		consume_rbkt();
		return e;
	}
	else {
		errorc("invalid syntax; expected identifier, literal, or grouping but got '%s'", cur()->lexeme);
	}
	return null;
}

#define MAKE_EXPR(x) expr* x = (expr*)malloc(sizeof(expr));

static expr* make_binary_expr(expr* left, expr* right, token* op) {
	MAKE_EXPR(new);
	new->type = EXPR_BINARY;
	new->binary.left = left;
	new->binary.right = right;
	new->binary.op = op;
	return new;
}

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

static int match(token_type t) {
	if (idx >= tokens_len) {
		return false;
	}

	if (cur()->type == t) {
		goto_next_tok();
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
	token* type = consume_identifier();
	/* TODO: match pointer * */
	
	data_type* new = (data_type*)malloc(sizeof(data_type));
	new->type = type;
	return new;
}

static void goto_next_tok(void) {
	if (idx == 0  || (idx - 1) < tokens_len) {
		++idx;
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

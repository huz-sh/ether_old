#include <ether/ether.h>

/**** PARSER STATE VARIABLES ****/
static file srcfile;
static token** tokens;
static uint64 tokens_len;

static uint64 idx;
static uint error_count;
static int error_occured;

/**** PARSER FUNCTIONS ****/
static expr* _expr(void);
static expr* expr_assign(void);
static expr* expr_add_sub(void);
static expr* expr_mul_div(void);
static expr* expr_primary(void);

static expr* make_binary_expr(expr*, expr*, token*);
static expr* make_number_expr(token*);
static expr* make_variable_expr(token*);

static int match(token_type);
static void expect(token_type, const char*, ...);

static void goto_next_tok(void);
static token* cur(void);
static token* prev(void);

static void errorc(const char*, ...);

void parser_init(file src, token** t) {
	srcfile = src;
	tokens = t;
	tokens_len = buf_len(t);
	idx = 0UL;
	error_count = 0;
	error_occured = false;
}

expr* parser_run(int* err) {
	expr* e = _expr();
	if (err) *err = error_occured;
	return e;
}

static expr* _expr(void) {
	expect(TOKEN_L_BKT, "expected '[' at start of expr");
	expr* e = expr_assign();
	expect(TOKEN_R_BKT, "expected ']' at end of expr");
	return e;
}

static expr* expr_assign(void) {
	/* TODO: parse actual assign expr */
	return expr_add_sub();
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
		expect(TOKEN_R_BKT, "expected ']' at end of grouping expr");
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

static void expect(token_type t, const char* msg, ...) {
	if (match(t)) {
		return;
	}

	va_list ap;
	va_start(ap, msg);
	errorc(msg, ap);
	va_end(ap);
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
	printf("%s:%ld:%d: error: ", srcfile.fpath, cur()->line, cur()->col);
	vprintf(msg, ap);
	va_end(ap);
	printf("\n");

	print_file_line_with_info(srcfile, cur()->line);
	print_marker_arrow_with_info_ln(srcfile, cur()->line, cur()->col);
	
	/* TODO: synchonization here */
	goto_next_tok(); /* TODO: remove this */

	error_occured = ETHER_ERROR;
	++error_count;
}

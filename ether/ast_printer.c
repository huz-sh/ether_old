#include <ether/ether.h>

/**** AST PRINTER STATE VARIABLES ****/
static uint ntab;

/**** AST PRINTER FUNCTIONS ****/
static void _stmt(stmt*);
static void _stmtn(stmt*);
static void func(stmt*);
static void var_decl(stmt*);
static void expr_stmt(stmt*);

static void _data_type(data_type*);

static void _token(token*);

static void add_tabs(void);

inline static void lbkt(void);
inline static void rbkt(void);
inline static void colon(void);
inline static void comma(void);
inline static void equal(void);
inline static void space(void);
inline static void newline(void);
inline static void addc(char);

static void _expr(expr*);
static void binary(expr*);
static void number(expr*);
static void variable(expr*);

void _print_ast(stmt** stmts) {
	ntab = 0;
	for (uint64 i = 0; i < buf_len(stmts); ++i) {
		_stmtn(stmts[i]);
	}
	printf("\n");
}

static void _stmt(stmt* s) {
	add_tabs();
	lbkt();
	switch (s->type) {
		case STMT_FUNC: func(s); break;
		case STMT_VAR_DECL: var_decl(s); break;
		case STMT_EXPR: expr_stmt(s); break;	
	}
	rbkt();
}

static void _stmtn(stmt* s) {
	_stmt(s);
	newline();
}

static void func(stmt* s) {
	_data_type(s->func.type);
	colon();
	space();
	_token(s->func.identifier);
	space();
	
	lbkt();
	param* p = s->func.params;
	size_t params_len = buf_len(s->func.params);
	for (uint64 i = 0; i < params_len; ++i) {
		_data_type(p[i].type);
		colon();
		_token(p[i].identifier);
		
		if (i < params_len - 1) {
			comma();
		}
	}
	rbkt();
	
	newline();

	ntab++;
	size_t body_len = buf_len(s->func.body);
	for (uint64 i = 0; i < body_len; ++i) {
		_stmt(s->func.body[i]);
		if (i < body_len - 1) {
			newline();
		}
	}
	ntab--;
}

static void var_decl(stmt* s) {
	_data_type(s->var_decl.type);
	colon();
	space();
	_token(s->var_decl.identifier);
	if (s->var_decl.initializer) {
		space();
		equal();
		space();
		_expr(s->var_decl.initializer);
	}
}

static void expr_stmt(stmt* s) {
	_expr(s->expr_stmt);		
}

static void _data_type(data_type* d) {
	_token(d->type);
	/* TODO: pointer */
}

static void add_tabs(void) {
	uint ctab = 0;

	while (ctab != ntab) {
		printf("    ");
		++ctab;
	}
}

inline static void _token(token* t) {
	assert(t);
	printf("%s", t->lexeme);	
}

inline static void lbkt(void) {
	addc('[');
}

inline static void rbkt(void) {
	addc(']');
}

inline static void colon(void) {
	addc(':');
}

inline static void comma(void) {
	addc(',');
}

inline static void equal(void) {
	addc('=');
}

inline static void space(void) {
	addc(' ');
}

inline static void newline(void) {
	addc('\n');
}

inline static void addc(char c) {
	printf("%c", c);
}

static void _expr(expr* e) {
	switch (e->type) {
		case EXPR_BINARY: binary(e); break;
		case EXPR_NUMBER: number(e); break;
		case EXPR_VARIABLE: variable(e); break;
		default: break;
	}
}

static void binary(expr* e) {
	printf("[");
	_expr(e->binary.left);
	printf(" %s ", e->binary.op->lexeme);
	_expr(e->binary.right);
	printf("]");
}

static void number(expr* e) {
	printf(e->number->lexeme);
}

static void variable(expr* e) {
	printf(e->variable->lexeme);
}

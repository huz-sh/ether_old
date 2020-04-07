#include <ether/ether.h>

/**** AST PRINTER STATE VARIABLES ****/
static uint ntab;

/**** AST PRINTER FUNCTIONS ****/
static void _stmt(stmt*);
static void _stmtn(stmt*);
static void _struct(stmt*);
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
static void number(expr*);
static void variable(expr*);
static void _func_call(expr*);

void _print_ast(stmt** stmts) {
	ntab = 0;
	for (uint64 i = 0; i < buf_len(stmts); ++i) {
		_stmtn(stmts[i]);
	}
	printf("\n");
}

static void _stmt(stmt* s) {
	add_tabs();

	if (s->type != STMT_EXPR) lbkt();
	
	switch (s->type) {
		case STMT_STRUCT:	 _struct(s); break;
		case STMT_FUNC:		func(s); break;
		case STMT_VAR_DECL: var_decl(s); break;
		case STMT_EXPR:		expr_stmt(s); break;
	}

	if (s->type != STMT_EXPR) rbkt();	
}

static void _stmtn(stmt* s) {
	_stmt(s);
	newline();
}

static void _struct(stmt* s) {
	printf("struct ");
	_token(s->_struct.identifier);

	struct_field** fields = s->_struct.fields;
	for (uint i = 0; i < buf_len(s->_struct.fields); ++i) {
		newline();
		printf("    ");
		lbkt();
		_data_type(fields[i]->type);
		colon();
		space();
		_token(fields[i]->identifier);
		rbkt();
	}
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
			space();
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
	printf("let ");
	_data_type(s->var_decl.type);
	colon();
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
		case EXPR_NUMBER: number(e); break;
		case EXPR_VARIABLE: variable(e); break;
		case EXPR_FUNC_CALL: _func_call(e); break;	
		default: break;
	}
}

static void number(expr* e) {
	printf(e->number->lexeme);
}

static void variable(expr* e) {
	printf(e->variable->lexeme);
}

static void _func_call(expr* e) {
	lbkt();
	size_t args_len = buf_len(e->func_call.args);
	_token(e->func_call.callee);
	if (args_len != 0) space();
	for (uint i = 0; i < args_len; ++i) {
		_expr(e->func_call.args[i]);
		if (i < args_len - 1) {
			space();
		}
	}
	rbkt();
}

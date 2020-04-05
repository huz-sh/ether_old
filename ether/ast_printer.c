#include <ether/ether.h>

static void ast(expr*);
static void binary(expr*);
static void number(expr*);
static void variable(expr*);

void _print_ast(expr* e) {
	ast(e);
	printf("\n");
}

static void ast(expr* e) {
	switch (e->type) {
		case EXPR_BINARY: binary(e); break;
		case EXPR_NUMBER: number(e); break;
		case EXPR_VARIABLE: variable(e); break;
		default: break;
	}
}

static void binary(expr* e) {
	printf("[");
	ast(e->binary.left);
	printf(" %s ", e->binary.op->lexeme);
	ast(e->binary.right);
	printf("]");
}

static void number(expr* e) {
	printf(e->number->lexeme);
}

static void variable(expr* e) {
	printf(e->variable->lexeme);
}

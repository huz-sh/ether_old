#include <ether/ether.h>

static void ast(expr* e);
static void binary(expr* e);
static void number(expr* e);

void print_ast(expr* e) {
	ast(e);
	printf("\n");

}

static void ast(expr* e) {
	switch (e->type) {
		case EXPR_BINARY: binary(e); break;
		case EXPR_NUMBER: number(e); break;
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

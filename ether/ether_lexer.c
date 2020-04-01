#ifndef __ETHER_LEXER_C
#define __ETHER_LEXER_C

typedef struct {
	file srcfile;
} lexer;

static
void lexer_init (lexer* l, file srcfile) {
	l->srcfile = srcfile;
}

static
void lexer_run(lexer* l) {

}

#endif

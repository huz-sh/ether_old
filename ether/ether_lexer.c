#ifndef __ETHER_LEXER_C
#define __ETHER_LEXER_C

typedef enum {
	TOKEN_L_BKT = '[', 
	TOKEN_R_BKT = ']',
} token_type;

typedef struct {
	token_type type;
} token;

typedef struct {
	file srcfile;
	token* tokens;
} lexer;

static
void lexer_init (lexer* l, file srcfile) {
	l->srcfile = srcfile;
	l->tokens = NULL;
}

static
void lexer_run(lexer* l) {
	for (uint i = 0; i < l->srcfile.len; ++i) {
		char c = l->srcfile.contents[i];
		switch (c) {
			case '[':
			case ']': {
				buf_push(l->tokens, (token){ (token_type)c });
			} break;
		}
	}
}

#endif

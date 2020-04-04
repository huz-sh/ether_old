#ifndef __LEXER_C
#define __LEXER_C

#include <ether/ether.h>

/**** LEXER STATE VARIABLES ****/
static file srcfile;
static token** tokens;

static char* start, *cur;
static uint64 line;

static uint error_count;
static int error_occured;

/**** LEXER FUNCTIONS ****/
static void identifier(void);
static void number(void);
static void string(void);
static void comment(void);

static void addt(token_type);
static int match(char);
static int at_end(void);
static void error(const char*, ...);

void lexer_init(file src) {
	srcfile = src;
	tokens = null;
	start = srcfile.contents;
	cur = srcfile.contents;
	line = 1;
	error_count = false;
	error_occured = false;
}

token** lexer_run(int* err) {
	for (cur = srcfile.contents; cur != (srcfile.contents + srcfile.len);) {
		start = cur;
		switch (*cur) {
			case ':': (match(':') ? addt(TOKEN_SCOPE) : addt(TOKEN_COLON)); break;

			case '+':
			case '-':
			case '*':
			case '/':	
			case '[':
			case ']': addt((token_type)(*cur)); break;

			case '"': string(); break;
				
			case '\t':
			case '\r':
			case ' ': ++cur; break;
				
			case '\n': {
				++line;
				++cur;
			} break;

			case ';': {
				if (match(';')) {
					comment();
				}
				else {
					error("invalid semicolon here; did you mean ';;'?");
					++cur;
				}
			} break;
				
			default: {
				if (isalpha(*cur) || (*cur) == '_') {
					identifier();
				}
				else if (isdigit(*cur)) {
					number();
				}
				else {
					error("invalid char literal '%c' in source code", *cur);
					++cur;
				}
			} break;
		}

		if (error_count > LEXER_ERROR_COUNT_MAX) {
			/* TODO: refactor note in function */
			printf("note: error count (%d) exceeded limit; aborting...\n", 
				   error_count);
			if (err) *err = error_occured;
			return tokens;
		}
	}

	if (err) *err = error_occured;
	return tokens;
}

static void identifier(void) {
	++cur;
	while (isalnum(*cur) || *cur == '_') {
		++cur;
	}
	--cur;
	addt(TOKEN_IDENTIFIER);
}

static void number(void) {
	while (isdigit(*cur)) {
		++cur;
	}
	
	if (*cur == '.') {
		++cur;
		
		int after_dot = false;
		while (isdigit(*cur)) {
			++cur;
			after_dot = true;
		}

		if (!after_dot) {
			error("expected digit after '.' in floating-point number");
		}
	}

	--cur;
	addt(TOKEN_NUMBER);
}

static void string(void) {
	++start;
	++cur;
	while ((*cur) != '"') {
		++cur;
	}
	--cur;
	addt(TOKEN_STRING);
	++cur;
}

static void comment(void) {
	while (*cur != '\n') {
		++cur;
	}
}

static void addt(token_type t) {
	token* new = (token*)malloc(sizeof(token));
	new->type = t;
	new->lexeme = strni(start, ++cur);
	new->line = line;
	buf_push(tokens, new);	
}

static int match(char c) {
	if (!at_end()) {
		if (*(cur+1) == c) {
			++cur;
			return true;
		}
	}
	return false;
}

static int at_end(void) {
	if (cur >= (srcfile.contents + srcfile.len)) {
		return true;
	}
	return false;
}

static void error(const char* msg, ...) {
	va_list ap;
	va_start(ap, msg);
	printf("%s:%ld: error: ", srcfile.fpath, line);
	vprintf(msg, ap);
	printf("\n");
	error_occured = LEXER_ERROR;
	++error_count;
}

#endif

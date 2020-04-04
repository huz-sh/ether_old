#ifndef __LEXER_C
#define __LEXER_C

#define LEXER_ERROR 1
#define LEXER_ERROR_COUNT_MAX 10

/**** LEXER TYPES ****/
typedef enum {
	TOKEN_L_BKT = '[', 
	TOKEN_R_BKT = ']',
	TOKEN_COLON = ':',
		
	TOKEN_IDENTIFIER = 1,
	TOKEN_NUMBER,
	TOKEN_STRING,
	TOKEN_SCOPE,
} token_type;

typedef struct {
	token_type type;
	char* lexeme;
	uint line;
} token;

/**** LEXER STATE VARIABLES ****/
static file srcfile;
static token* tokens;

static char* start, *cur;
static uint line;

static uint error_count;
static int error_occured;

/**** LEXER FUNCTIONS ****/
void lexer_init(file srcfile);
int run(void);

static void identifier(void);
static void number(void);
static void string(void);
static void comment(void);

static void addt(token_type t);
static int match(char c);
static int at_end(void);
static void error(const char* msg, ...);

void lexer_init(file src) {
	srcfile = src;
	start = srcfile.contents;
	cur = srcfile.contents;
	line = 1;
	error_count = 0;
	error_occured = 0;
}

token* lexer_run(int* err) {
	for (cur = srcfile.contents; cur != (srcfile.contents + srcfile.len);) {
		start = cur;
		switch (*cur) {
			case ':': (match(':') ? addt(TOKEN_SCOPE) : addt(TOKEN_COLON)); break;
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
		
		int after_dot = 0;
		while (isdigit(*cur)) {
			++cur;
			after_dot = 1;
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
	buf_push(tokens, (token){t, strni(start, ++cur), line });	
}

static int match(char c) {
	if (!at_end()) {
		if (*(cur+1) == c) {
			++cur;
			return 1;
		}
	}
	return 0;
}

static int at_end(void) {
	if (cur >= (srcfile.contents + srcfile.len)) {
		return 1;
	}
	return 0;
}

static void error(const char* msg, ...) {
	va_list ap;
	va_start(ap, msg);
	printf("%s:%d: error: ", srcfile.fpath, line);
	vprintf(msg, ap);
	printf("\n");
	error_occured = LEXER_ERROR;
	++error_count;
}

#endif

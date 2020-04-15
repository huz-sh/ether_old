#include <ether/ether.h>

static SourceFile* srcfile;
static Token** tokens;
static char** keywords;

static char* start, *cur;
static u64 line;
static char* last_newline;
static char* last_to_last_newline;

static uint	 error_count;
static error_code error_occured;

static void lexer_destroy(void);

static void lex_identifier(void);
static void lex_number(void);
static void lex_string(void);
static void lex_comment(void);
static void lex_newline(void);

static void add_token(TokenType);
static void add_eof(void);

static bool match_char(char);
static bool is_at_end(void);
static u32 get_column(void);
static void error_at_current(const char*, ...);

void lexer_init(SourceFile* src) {
	srcfile = src;
	tokens = null;
	start = srcfile->contents;
	cur = srcfile->contents;
	line = 1;
	error_count = 0;
	error_occured = false;
	last_newline = srcfile->contents;
	last_to_last_newline = null;

	buf_push(keywords, str_intern("struct"));
	buf_push(keywords, str_intern("defn"));
	buf_push(keywords, str_intern("decl"));	
	buf_push(keywords, str_intern("let"));
	buf_push(keywords, str_intern("if"));
	buf_push(keywords, str_intern("elif"));
	buf_push(keywords, str_intern("else"));	
	buf_push(keywords, str_intern("set"));

	buf_push(keywords, str_intern("int"));
	buf_push(keywords, str_intern("char"));
	buf_push(keywords, str_intern("bool"));
	buf_push(keywords, str_intern("void"));
}

Token** lexer_run(error_code* out_error_code) {
	for (cur = srcfile->contents; cur != (srcfile->contents + srcfile->len);) {
		start = cur;
		switch (*cur) {
			case ':': add_token(TOKEN_COLON); break;
			case '+': add_token(TOKEN_PLUS);  break;
			case '-': add_token(TOKEN_MINUS); break;
			case '*': add_token(TOKEN_STAR);  break;
			case '/': add_token(TOKEN_SLASH); break;
			case '[': add_token(TOKEN_LEFT_BRACKET); break;
			case ']': add_token(TOKEN_RIGHT_BRACKET); break;
			case '=': add_token(TOKEN_EQUAL); break;
			case ',': add_token(TOKEN_COMMA); break;
				
			case '"':  lex_string(); break;
			case '\n': lex_newline(); break;

			case '\t':
			case '\r':
			case ' ': ++cur; break;

			case ';': {
				if (match_char(';')) {
					lex_comment();
				}
				else {
					error_at_current("invalid semicolon here; did you mean ';;'?");
					++cur;
				}
			} break;
			
			case 'A': case 'B': case 'C': case 'D':
			case 'E': case 'F': case 'G': case 'H':
			case 'I': case 'J': case 'K': case 'L':
			case 'M': case 'N': case 'O': case 'P':
			case 'Q': case 'R': case 'S': case 'T':
			case 'U': case 'V': case 'W': case 'X':
			case 'Y': case 'Z': 
			case 'a': case 'b': case 'c': case 'd':
			case 'e': case 'f': case 'g': case 'h':
			case 'i': case 'j': case 'k': case 'l':
			case 'm': case 'n': case 'o': case 'p':
			case 'q': case 'r': case 's': case 't':
			case 'u': case 'v': case 'w': case 'x':
			case 'y': case 'z':
			case '_':
				lex_identifier();
				break;

			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				lex_number();
				break;
				
			default: {
				error_at_current("invalid char literal '%c' (dec: %d)", *cur, (int)(*cur));
				++cur;
			} break;
		}

		if (error_count > LEXER_ERROR_COUNT_MAX) {
			/* TODO: refactor note in function */
			printf("note: error count (%d) exceeded limit; aborting...\n",
				   error_count);
			if (out_error_code) *out_error_code = error_occured;
			return tokens;
		}
	}

	if (out_error_code) *out_error_code = error_occured;
	add_eof();
	lexer_destroy();
	return tokens;
}

static void lexer_destroy(void) {
	buf_free(keywords);
}

static void lex_identifier(void) {
	++cur;
	while (isalnum(*cur) || *cur == '_') {
		++cur;
	}

	u64 keywords_len = buf_len(keywords);
	char* keyword = str_intern_range(start, cur);
	bool is_keyword = false;
	for (uint i = 0; i < keywords_len; ++i) {
		if (keyword == str_intern(keywords[i])) {
			is_keyword = true;
		}
	}

	Token* new = (Token*)malloc(sizeof(Token));
	new->type = is_keyword ? TOKEN_KEYWORD : TOKEN_IDENTIFIER;
	new->lexeme = keyword;
	new->srcfile = srcfile;
	new->line = line;
	new->column = get_column();
	buf_push(tokens, new);
}

static void lex_number(void) {
	while (isdigit(*cur)) {
		++cur;
	}

	if (*cur == '.') {
		++cur;
		bool after_dot = false;
		while (isdigit(*cur)) {
			++cur;
			after_dot = true;
		}
		if (!after_dot) {
			error_at_current("expected digit after '.' in floating-point number");
		}
	}

	--cur;
	add_token(TOKEN_NUMBER);
}

static void lex_string(void) {
	++start;
	++cur;
	while ((*cur) != '"') {
		if (*cur == '\0') {
			error_at_current("missing terminating '\"'");
		}
		++cur;
	}
	--cur;
	add_token(TOKEN_STRING);
	++cur;
}

static void lex_comment(void) {
	while (*cur != '\n') {
		if (*cur == '\0') return;
		++cur;
	}
}

static void lex_newline(void) {
	last_to_last_newline = last_newline;
	last_newline = cur;
	++line;
	++cur;
}

static void add_token(TokenType type) {
	Token* new = (Token*)malloc(sizeof(Token));
	new->type = type;
	new->lexeme = str_intern_range(start, ++cur);
	new->srcfile = srcfile;
	new->line = line;
	new->column = get_column();
	buf_push(tokens, new);
}

static void add_eof(void) {
	bool newline = false;
	u64 eof_line = line;
	if (*(cur - 1) == '\n') {
		newline = true;
		--eof_line;
	}

	Token* t = (Token*)malloc(sizeof(Token));
	t->type = TOKEN_EOF;
	t->lexeme = "";
	t->srcfile = srcfile;
	t->line = eof_line;
	if (newline) t->column = cur - last_to_last_newline - 1;
	else t->column = cur - last_newline;
	buf_push(tokens, t);
}

static bool match_char(char c) {
	if (!is_at_end()) {
		if (*(cur + 1) == c) {
			++cur;
			return true;
		}
	}
	return false;
}

static bool is_at_end(void) {
	if (cur >= (srcfile->contents + srcfile->len)) {
		return true;
	}
	return false;
}

static u32 get_column(void) {
	u32 column = (start - last_newline);
	column = (line == 1 ? column + 1 : column);
	return column;
}

static void error_at_current(const char* msg, ...) {
	u32 column = get_column();
	va_list ap;
	va_start(ap, msg);
	printf("%s:%ld:%d: error: ", srcfile->fpath, line, column);
	vprintf(msg, ap);
	printf("\n");
	va_end(ap);

	print_file_line_with_info(srcfile, line);
	print_marker_arrow_with_info_ln(srcfile, line, column);

	error_occured = ETHER_ERROR;
	++error_count;
}

#include <ether/ether.h>

static void lexer_destroy(Lexer*);

static void lex_identifier(Lexer*);
static void lex_number(Lexer*);
static void lex_string(Lexer*);
static void lex_char(Lexer*);
static void lex_comment(Lexer*);
static void lex_newline(Lexer*);

static void add_token(Lexer*, TokenType);
static void add_eof(Lexer*);

static bool match_char(Lexer*, char);
static bool is_at_end(Lexer*);
static u32 get_column(Lexer*);
static void error_at_current(Lexer*, const char*, ...);

static void init_keywords(Lexer*);

Token** lexer_run(Lexer* l, SourceFile* file, error_code* out_error_code) {
	l->srcfile = file;
	l->tokens = null;
	l->start = l->srcfile->contents;
	l->cur = l->srcfile->contents;
	l->line = 1;
	l->error_count = 0;
	l->error_occured = false;
	l->last_newline = l->srcfile->contents;
	l->last_to_last_newline = null;
	l->keywords = null;

	init_keywords(l);

	for (l->cur = l->srcfile->contents; l->cur != (l->srcfile->contents + l->srcfile->len);) {
		l->start = l->cur;
		switch (*l->cur) {
			case ':': add_token(l, TOKEN_COLON); break;
			case '+': add_token(l, TOKEN_PLUS);  break;
			case '-': add_token(l, TOKEN_MINUS); break;
			case '*': add_token(l, TOKEN_STAR);  break;
			case '/': add_token(l, TOKEN_SLASH); break;
			case '[': add_token(l, TOKEN_LEFT_BRACKET); break;
			case ']': add_token(l, TOKEN_RIGHT_BRACKET); break;
			case '=': add_token(l, TOKEN_EQUAL); break;
			case ',': add_token(l, TOKEN_COMMA); break;
			case '.': add_token(l, TOKEN_DOT); break;
			case '<': match_char(l, '=') ? add_token(l, TOKEN_LESS_EQUAL) : add_token(l, TOKEN_LESS); break;
			case '>': match_char(l, '=') ? add_token(l, TOKEN_GREATER_EQUAL) : add_token(l, TOKEN_GREATER); break;
				
			case '"':  lex_string(l); break;
			case '\'': lex_char(l); break;	
			case '\n': lex_newline(l); break;

			case '\t':
			case '\r':
			case ' ': ++l->cur; break;

			case ';': {
				if (match_char(l, ';')) {
					lex_comment(l);
				}
				else {
					error_at_current(l, "invalid semicolon here; did you mean ';;'?");
					++l->cur;
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
				lex_identifier(l);
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
				lex_number(l);
				break;
				
			default: {
				error_at_current(l, "invalid char literal '%c' (dec: %d)", *l->cur, (int)(*l->cur));
				++l->cur;
			} break;
		}

		if (l->error_count > LEXER_ERROR_COUNT_MAX) {
			/* TODO: refactor note in function */
			printf("note: error count (%d) exceeded limit; aborting...\n",
				   l->error_count);
			if (out_error_code) *out_error_code = l->error_occured;
			return l->tokens;
		}
	}

	if (out_error_code) *out_error_code = l->error_occured;
	add_eof(l);
	lexer_destroy(l);
	return l->tokens;
}

static void lexer_destroy(Lexer* l) {
	buf_free(l->keywords);
}

static void lex_identifier(Lexer* l) {
	++l->cur;
	while (isalnum(*l->cur) || *l->cur == '_') {
		++l->cur;
	}

	u64 keywords_len = buf_len(l->keywords);
	char* keyword = str_intern_range(l->start, l->cur);
	bool is_keyword = false;
	for (uint i = 0; i < keywords_len; ++i) {
		if (keyword == str_intern(l->keywords[i])) {
			is_keyword = true;
		}
	}

	Token* new = (Token*)malloc(sizeof(Token));
	new->type = is_keyword ? TOKEN_KEYWORD : TOKEN_IDENTIFIER;
	new->lexeme = keyword;
	new->srcfile = l->srcfile;
	new->line = l->line;
	new->column = get_column(l);
	buf_push(l->tokens, new);
}

static void lex_number(Lexer* l) {
	while (isdigit(*l->cur)) {
		++l->cur;
	}

	if (*l->cur == '.') {
		++l->cur;
		bool after_dot = false;
		while (isdigit(*l->cur)) {
			++l->cur;
			after_dot = true;
		}
		if (!after_dot) {
			error_at_current(l, "expected digit after '.' in floating-point number");
		}
	}

	--l->cur;
	add_token(l, TOKEN_NUMBER);
}

static void lex_string(Lexer* l) {
	++l->start;
	++l->cur;
	while ((*l->cur) != '"') {
		if (*l->cur == '\0') {
			error_at_current(l, "missing terminating '\"'");
		}
		++l->cur;
	}
	--l->cur;
	add_token(l, TOKEN_STRING);
	++l->cur;
}

static void lex_char(Lexer* l) {
	++l->start;
	l->cur += 2;
	/* TODO: escape sequences */
	if ((*l->cur) != '\'') {
		error_at_current(l, "missing terminating \"'\"");
	}
	--l->cur;
	add_token(l, TOKEN_CHAR);
	++l->cur;
}

static void lex_comment(Lexer* l) {
	while (*l->cur != '\n') {
		if (*l->cur == '\0') return;
		++l->cur;
	}
}

static void lex_newline(Lexer* l) {
	l->last_to_last_newline = l->last_newline;
	l->last_newline = l->cur;
	++l->line;
	++l->cur;
}

static void add_token(Lexer* l, TokenType type) {
	Token* new = (Token*)malloc(sizeof(Token));
	new->type = type;
	new->lexeme = str_intern_range(l->start, ++l->cur);
	new->srcfile = l->srcfile;
	new->line = l->line;
	new->column = get_column(l);
	buf_push(l->tokens, new);
}

static void add_eof(Lexer* l) {
	bool newline = false;
	u64 eof_line = l->line;
	if (*(l->cur - 1) == '\n') {
		newline = true;
		--eof_line;
	}

	Token* t = (Token*)malloc(sizeof(Token));
	t->type = TOKEN_EOF;
	t->lexeme = "";
	t->srcfile = l->srcfile;
	t->line = eof_line;
	if (newline) t->column = l->cur - l->last_to_last_newline - 1;
	else t->column = l->cur - l->last_newline;
	buf_push(l->tokens, t);
}

static bool match_char(Lexer* l, char c) {
	if (!is_at_end(l)) {
		if (*(l->cur + 1) == c) {
			++l->cur;
			return true;
		}
	}
	return false;
}

static bool is_at_end(Lexer* l) {
	if (l->cur >= (l->srcfile->contents + l->srcfile->len)) {
		return true;
	}
	return false;
}

static u32 get_column(Lexer* l) {
	u32 column = (l->start - l->last_newline);
	column = (l->line == 1 ? column + 1 : column);
	return column;
}

static void error_at_current(Lexer* l, const char* msg, ...) {
	u32 column = get_column(l);
	va_list ap;
	va_start(ap, msg);
	printf("%s:%ld:%d: error: ", l->srcfile->fpath, l->line, column);
	vprintf(msg, ap);
	printf("\n");
	va_end(ap);

	print_file_line_with_info(l->srcfile, l->line);
	print_marker_arrow_with_info_ln(l->srcfile, l->line, column);

	l->error_occured = ETHER_ERROR;
	++l->error_count;
}

static void init_keywords(Lexer* l) {
	buf_push(l->keywords, str_intern("struct"));
	buf_push(l->keywords, str_intern("defn"));
	buf_push(l->keywords, str_intern("decl"));
	buf_push(l->keywords, str_intern("pub"));	
	buf_push(l->keywords, str_intern("load"));
	buf_push(l->keywords, str_intern("let"));
	buf_push(l->keywords, str_intern("if"));
	buf_push(l->keywords, str_intern("elif"));
	buf_push(l->keywords, str_intern("else"));
	buf_push(l->keywords, str_intern("for"));
	buf_push(l->keywords, str_intern("to"));
	buf_push(l->keywords, str_intern("return"));	
	buf_push(l->keywords, str_intern("set"));
	buf_push(l->keywords, str_intern("deref"));
	buf_push(l->keywords, str_intern("addr"));
	buf_push(l->keywords, str_intern("at"));
	
	buf_push(l->keywords, str_intern("int"));
	buf_push(l->keywords, str_intern("char"));
	buf_push(l->keywords, str_intern("bool"));
	buf_push(l->keywords, str_intern("void"));
}

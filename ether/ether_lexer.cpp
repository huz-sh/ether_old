#pragma once

#define LEXER_ERROR 1
#define LEXER_ERROR_COUNT_MAX 4

namespace ether {
	enum token_type {
		TOKEN_L_BKT = '[', 
		TOKEN_R_BKT = ']',
		TOKEN_COLON = ':',
		
		TOKEN_IDENTIFIER = 1,
		TOKEN_NUMBER,
		TOKEN_STRING,
		TOKEN_SCOPE,
	};

	struct token {
		token_type type;
		char* lexeme;
		uint line;
	};

	struct lexer {
		io::file srcfile;
		arr<token> tokens;

		lexer (io::file srcfile);
		int run(void);

	private:
		void identifier(void);
		void number(void);
		void string(void);
		void comment(void);

		void addt(token_type t);

		int match(char c);
		int at_end(void);
		void error(const char* msg, ...);
		
		char* start, *cur;
		uint line;
		
		uint error_count;
		int error_occured;
	};
}

ether::lexer::lexer (io::file srcfile) {
	this->srcfile = srcfile;
	this->error_occured = 0;
	this->start = srcfile.contents;
	this->cur = srcfile.contents;
	this->error_count = 0;
}

int ether::lexer::run(void) {
	line = 1;
	for (cur = this->srcfile.contents; cur != (this->srcfile.contents + this->srcfile.len);) {
		start = cur;
		switch (*cur) {
			case ':': (match(':') ? addt(TOKEN_SCOPE) : addt(TOKEN_COLON)); break;
			case '[':
			case ']': addt((ether::token_type)(*cur)); break;

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
			return this->error_occured;
		}
	}
	
	return this->error_occured;
}

void ether::lexer::identifier(void) {
	++cur;
	while (isalnum(*cur) || *cur == '_') {
		++cur;
	}
	--cur;
	addt(TOKEN_IDENTIFIER);
}

void ether::lexer::number(void) {
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

void ether::lexer::string(void) {
	++start;
	++cur;
	while ((*cur) != '"') {
		++cur;
	}
	--cur;
	addt(TOKEN_STRING);
	++cur;
}

void ether::lexer::comment(void) {
	while (*cur != '\n') {
		++cur;
	}
}

void ether::lexer::addt(ether::token_type t) {
	tokens.push((token){t, strni(start, ++cur), line });	
}

int ether::lexer::match(char c) {
	if (!at_end()) {
		if (*(cur+1) == c) {
			++cur;
			return 1;
		}
	}
	return 0;
}

int ether::lexer::at_end(void) {
	if (cur >= (this->srcfile.contents + this->srcfile.len)) {
		return 1;
	}
	return 0;
}

void ether::lexer::error(const char* msg, ...) {
	va_list ap;
	va_start(ap, msg);
	printf("%s:%d: error: ", this->srcfile.fpath, line);
	vprintf(msg, ap);
	printf("\n");
	this->error_occured = LEXER_ERROR;
	++error_count;
}

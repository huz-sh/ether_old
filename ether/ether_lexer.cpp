#pragma once

namespace ether {
	enum token_type {
		TOKEN_L_BKT = '[', 
		TOKEN_R_BKT = ']',
	};

	struct token {
		token_type type;
		uint line;
		uint col;
	};

	struct lexer {
		io::file srcfile;
		arr<token> tokens;

		lexer (io::file srcfile);
		void run(void);
	};
}

ether::lexer::lexer (io::file srcfile) {
	this->srcfile = srcfile;
}

void ether::lexer::run(void) {
	uint line = 1;
	uint col = 1;
	for (uint i = 0; i < this->srcfile.len; ++i, ++col) {
		char c = this->srcfile.contents[i];
		switch (c) {
			case '[':
			case ']': {
				tokens.push((token){ (token_type)c, line, col });
			} break;

			case '\t': {
				col += 3;	/* col goes to 4 in the next iteration */
			} break;

			case '\n': {
				++line;
				col = 0;	/* col increments to 1 in the next iteration */
			} break;
								
			case '#': {
				while (this->srcfile.contents[i] != '\n') {
					++i; ++col;
				}
				--i; --col;
			} break;

			default: break;
		}
	}
}

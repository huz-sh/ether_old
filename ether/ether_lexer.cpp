#pragma once

namespace ether {
	enum token_type {
		TOKEN_L_BKT = '[', 
		TOKEN_R_BKT = ']',
	};

	struct token {
		token_type type;
		uint col;
		uint line;
	};

	struct lexer {
		io::file srcfile;
		arr<token> tokens;

		lexer (io::file srcfile) {
			this->srcfile = srcfile;
		}

		void run(void) {
			uint line = 1;
			uint col = 1;
			for (uint i = 0; i < this->srcfile.len; ++i, ++col) {
				char c = this->srcfile.contents[i];
				switch (c) {
					case '[':
					case ']': {
						tokens.push((token){ (token_type)c,  });
					} break;

					case '\n':
					case '\r': {
						++line;
						col = 1;
					} break;
								
					case '#': {
						while (this->srcfile.contents[i] != '\n' &&
							   this->srcfile.contents[i] != '\r') {
							++i; ++col;
						}
					} break;

					default: break;
				}
			}
		}
	};
}

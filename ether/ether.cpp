#include <assert.h>
#include <stddef.h>
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstdarg>

#include <ether/typedefs.h>

#include "ether_math.cpp"
#include "ether_buf.cpp"
#include "ether_io.cpp"
#include "ether_error.cpp"
#include "ether_lexer.cpp"

int main (int argc, char** argv) {
	if (argc < 2) {
		ether::error("no input files supplied");			
	}

	/* TODO: check if we have to free this pointer.
	 * is it expensive to keep it around? */
	ether::io::file srcfile = ether::io::read_file(argv[1]);
	if (!srcfile.contents) {
		ether::error("%s: no such file or directory", argv[1]);
	}
	
	ether::lexer l(srcfile);
	l.run();
	for (uint i = 0; i < l.tokens.len; ++i) {
		printf("token: %c at line %d, col %d\n", l.tokens[i].type, l.tokens[i].line, l.tokens[i].col);
	}
}

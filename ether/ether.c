#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#include <ether/typedefs.h>

#include "ether_math.c"
#include "ether_buf.c"
#include "ether_str_intern.c"
#include "ether_io.c"
#include "ether_error.c"
#include "ether_lexer.c"

int main (int argc, char** argv) {
	if (argc < 2) {
		ether_error("no input files supplied");			
	}

	/* TODO: check if we have to free this pointer.
	 * is it expensive to keep it around? */
	file srcfile = ether_read_file(argv[1]);
	if (!srcfile.contents) {
		ether_error("%s: no such file or directory", argv[1]);
	}
	
	lexer_init(srcfile);
	int err = 0;
	token* tokens = lexer_run(&err);
	if (err == LEXER_ERROR) ether_error("compilation aborted.");
	
	for (uint i = 0; i < buf_len(tokens); ++i) {
		printf("token: '%s' at line %d\n",
			   (tokens[i].lexeme), tokens[i].line);
	}
}

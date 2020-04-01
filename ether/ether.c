#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>

#include <ether/typedefs.h>

#include "ether_io.c"
#include "ether_error.c"
#include "ether_lexer.c"

int main (int argc, char** argv) {
	if (argc < 2) {
		ether_error("no input files supplied");			
	}

	/* TODO: check if we have to free this pointer.
	 * is it expensive to keep it around? */
	file srcfile = read_file(argv[1]);
	if (!srcfile.contents) {
		ether_error("%s: no such file or directory", argv[1]);
	}
	
	lexer l = { };
	lexer_init(&l, srcfile);
	lexer_run(&l);
}

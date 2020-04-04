#include <ether/ether.h>

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
	int err = false;
	token** tokens = lexer_run(&err);
	if (err == LEXER_ERROR) ether_error("compilation aborted.");
	
	for (uint i = 0; i < buf_len(tokens); ++i) {
		printf("token: '%s' at line %ld\n",
			   (tokens[i]->lexeme), tokens[i]->line);
	}

	parser_init(srcfile, tokens);
	expr* e = parser_run(&err);
}

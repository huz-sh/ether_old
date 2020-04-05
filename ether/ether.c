#include <ether/ether.h>

static void quit(void);

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
	if (err == ETHER_ERROR) quit();

	printf("--- TOKENS ---\n");
	for (uint i = 0; i < buf_len(tokens); ++i) {
		printf("token: '%s' at line %ld, col %d\n",
			   (tokens[i]->lexeme), tokens[i]->line, tokens[i]->col);
	}
	printf("--- END ---\n");

	parser_init(srcfile, tokens);
	stmt** stmts = parser_run(&err);
	if (err == ETHER_ERROR) quit();

	printf("\n--- AST ---\n");
	printf("--- END ---\n");
}

static void quit(void) {
	ether_error("compilation aborted.");
}

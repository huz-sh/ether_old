#include <ether/ether.h>

#define PRINT_TOKENS 0
#define PRINT_AST	 0

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

#if PRINT_TOKENS
	printf("--- TOKENS ---\n");
	for (uint i = 0; i < buf_len(tokens); ++i) {
		printf("token: '%s' at line %ld, col %d\n",
			   (tokens[i]->lexeme), tokens[i]->line, tokens[i]->col);
	}
	printf("--- END ---\n\n");
#endif

	parser_init(srcfile, tokens);
	stmt** stmts = parser_run(&err);
	if (err == ETHER_ERROR) quit();

#if PRINT_AST
	printf("--- AST ---\n");
	printf("--- END ---\n");
#endif
}

static void quit(void) {
	ether_error("compilation aborted.");
}

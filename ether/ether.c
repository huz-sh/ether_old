#include <ether/ether.h>

#define PRINT_TOKENS 0
#define PRINT_AST	 1

inline static void quit(void);

int main(int argc, char** argv) {
	/* TODO: check file extension */

	/* TODO: check if we have to free this pointer.
	 * is it expensive to keep it around? */
	SourceFile* srcfile = ether_read_file(argv[1]);
	if (!srcfile->contents) {
		ether_error("%s: no such file or directory", argv[1]);
	}

	error_code err = false;
	Lexer lexer;
	Token** tokens = lexer_run(&lexer, srcfile, &err);
	if (err == ETHER_ERROR) quit();

#if PRINT_TOKENS
	printf("--- TOKENS ---\n");
	for (uint i = 0; i < buf_len(tokens); ++i) {
		printf("token: '%s' (%d) at line %ld, col %d\n",
			   (tokens[i]->lexeme), tokens[i]->type,
			   tokens[i]->line, tokens[i]->column);
	}
	printf("--- END ---\n\n");
#endif

	err = false;
	parser_init(tokens, srcfile);
	Stmt** stmts = parser_run(&err);
	if (err == ETHER_ERROR) quit();

#if PRINT_AST
	printf("--- AST ---\n");
	print_ast(stmts);
	printf("--- END ---\n");
#endif

	linker_init(stmts);
	Stmt** structs = linker_run(&err);
	if (err == ETHER_ERROR) quit();

	resolve_init(stmts, structs);
	err = resolve_run();
	if (err == ETHER_ERROR) quit();

	code_gen_init(stmts, srcfile);
	code_gen_run();
}

inline static void quit(void) {
	ether_error("compilation aborted.");
}

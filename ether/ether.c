#include <ether/ether.h>

#define PRINT_TOKENS 0
#define PRINT_AST	 1

inline static void quit(void);

int main(int argc, char** argv) {
	if (argc < 2) {
		ether_error("no input files supplied");
	}

	SourceFile* srcfiles = null;
	Stmt*** stmts_buf = null;
	
	/* TODO: check if we have to free this pointer.
	 * is it expensive to keep it around? */
	SourceFile srcfile = ether_read_file(argv[1]);
	if (!srcfile.contents) {
		ether_error("%s: no such file or directory", argv[1]);
	}
	buf_push(srcfiles, srcfile);

	lexer_init(srcfile);
	error_code err = false;
	Token** tokens = lexer_run(&err);
	if (err == ETHER_ERROR) quit();

#if PRINT_TOKENS
	printf("--- TOKENS ---\n");
	for (uint i = 0; i < buf_len(tokens); ++i) {
		printf("token: '%s' (%d) at line %ld, col %d\n",
			   (tokens[i]->lexeme), tokens[i]->type,
			   tokens[i]->line, tokens[i]->col);
	}
	printf("--- END ---\n\n");
#endif

	err = false;
	parser_init(srcfile, tokens);
	Stmt** stmts = parser_run(&err);
	if (err == ETHER_ERROR) quit();
	buf_push(stmts_buf, stmts);

#if PRINT_AST
	printf("--- AST ---\n");
	print_ast(stmts);
	printf("--- END ---\n");
#endif

	linker_init(srcfiles, stmts_buf);
	err = linker_run();
	if (err == ETHER_ERROR) quit();
}

inline static void quit(void) {
	ether_error("compilation aborted.");
}

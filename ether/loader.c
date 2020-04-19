#include <ether/ether.h>

static SourceFile* srcfile;
static Stmt** stmts;

error_code loader_load(SourceFile* file, Stmt** existing_stmts) {
	srcfile = file;
	stmts = existing_stmts;
}

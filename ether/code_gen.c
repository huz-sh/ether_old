#include <ether/ether.h>

/* 
FILE* popen(const char*, const char*);
int pclose(FILE*);

FILE* gcc;
gcc = popen("gcc -xc -", "w");
if (!gcc) {
	printf("error");
	return;
}
fputs("int main(void) { return 0; }", gcc);
pclose(gcc);
*/

static Stmt*** stmts_all;
static char* output_code;

static void code_gen_destroy(void);

static void gen_structs(void);
static void gen_global_var_decls(void);
static void gen_function_prototypes(void);
static void gen_file(Stmt**);
static void gen_func(Stmt*);
static void gen_stmt(Stmt*);
static void gen_var_decl(Stmt*);
static void gen_if_stmt(Stmt*);
static void gen_expr_stmt(Stmt*);

void code_gen_init(Stmt*** stmts_buf) {
	stmts_all = stmts_buf;
	output_code = null;	
}

void code_gen_run(void) {
	gen_structs();
	gen_global_var_decls();
	gen_function_prototypes();
	
	for (u64 i = 0; i < buf_len(stmts_all); ++i) {
		gen_file(stmts_all[i]);
	}
	code_gen_destroy();
}

static void code_gen_destroy(void) {
}

static void gen_structs(void) {
}

static void gen_global_var_decls(void) {
}

static void gen_function_prototypes(void) {
}

static void gen_file(Stmt** stmts) {
	for (u64 i = 0; i < buf_len(stmts); ++i) {
		if (stmts[i]->type == STMT_FUNC) {
			gen_func(stmts[i]);
		}
	}	
}

static void gen_func(Stmt* stmt) {
}

static void gen_stmt(Stmt* stmt) {
	switch (stmt->type) {
		case STMT_VAR_DECL: gen_var_decl(stmt); break;
		case STMT_IF: gen_if_stmt(stmt); break;	
		case STMT_EXPR: gen_expr_stmt(stmt); break;
		case STMT_STRUCT:
		case STMT_FUNC: break;	
	}
}

static void gen_var_decl(Stmt* stmt) {
}

static void gen_if_stmt(Stmt* stmt) {
}

static void gen_expr_stmt(Stmt* stmt) {
}

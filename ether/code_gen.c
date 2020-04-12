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

void code_gen_init(Stmt*** stmts_buf) {
	stmts_all = stmts_buf;
}

void code_gen_run(void) {
	
}

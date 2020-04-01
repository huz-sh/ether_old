#include <stdio.h>
#include <stdarg.h>

#define ether_error(x, ...) _ether_error(__FILE__, __LINE__, \
										 x, ##__VA_ARGS__)

static
void _ether_error(const char* file, int line, const char* fmt, ...) {
	printf("ether: ");

#ifdef _DEBUG
	printf("(%s:%d) ", file, line);
#endif

	va_list ap;
	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);
	printf("\n");
}

typedef struct {
	
} lexer;

int main(int argc, char** argv) {
	if (argc < 2) {
		ether_error("no input files supplied.");			
	}
	
	//lexer l = { };
	//lexer_init(&l);
}

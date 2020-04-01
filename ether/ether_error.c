#ifndef __ETHER_ERROR_C
#define __ETHER_ERROR_C

#define ether_error(x, ...) _ether_error(__FILE__, __LINE__, \
										 x, ##__VA_ARGS__)

static
void _ether_error (const char* fpath, int line, const char* fmt, ...) {
	printf("ether: ");

#ifdef _DEBUG
	printf("(%s:%d) ", fpath, line);
#endif

	va_list ap;
	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);
	printf("\n");

	exit(EXIT_FAILURE);
}

#endif

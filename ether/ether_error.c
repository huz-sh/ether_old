#ifndef __ERROR_C
#define __ERROR_C

void ether_error(const char* fmt, ...) {
	printf("ether: ");

	va_list ap;
	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);
	printf("\n");

	exit(EXIT_FAILURE);
}

#endif 

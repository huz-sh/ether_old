int num_len(int);
void* malloc(int);
void sputd(char*, int);
void puts(char*);
void free(void*);

void putd(int d) {
	int len = num_len(d);
	char* buf = malloc(len + 1);

	sputd(buf, d);
	puts(buf);
	free(buf);
}

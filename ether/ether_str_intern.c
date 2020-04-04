#ifndef __STR_INTERN_C
#define __STR_INTERN_C

typedef struct {
	size_t len;
	char* str;
} intern;

static intern* interns;

char* strni(char* start, char* end) {
	size_t len = end - start;
	for (size_t i = 0; i < buf_len(interns); ++i) {
		if (interns[i].len == len &&
			strncmp(interns[i].str, start, len) == 0) {
			return interns[i].str;
		}
	}

	char* str = (char*)malloc(len + 1);
	memcpy(str, start, len);
	str[len] = 0;
	buf_push(interns, (intern){ len, str });
	return str;
}

char* stri(char* str) {
	return strni(str, str + strlen(str));
}

#endif

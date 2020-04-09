#include <ether/ether.h>

static Intern* interns;

char* str_intern_range(char* start, char* end) {
	u64 len = end - start;
	for (u64 i = 0; i < buf_len(interns); ++i) {
		if (interns[i].len == len &&
			strncmp(interns[i].str, start, len) == false) {
			return interns[i].str;
		}
	}

	char* str = (char*)malloc(len + 1);
	memcpy(str, start, len);
	str[len] = 0;
	buf_push(interns, (Intern){ len, str });
	return str;
}

char* str_intern(char* str) {
	return str_intern_range(str, str + strlen(str));
}

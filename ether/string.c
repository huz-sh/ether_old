#include <ether/ether.h>

echar* estr_create(char* str) {
	echar* buf = null;
	for (u64 i = 0; i < strlen(str); ++i) {
		buf_push(buf, str[i]);		
	}
	buf_push(buf, '\0');
	return buf;
}

u64 estr_len(echar* estr) {
	return buf_len(estr);
}

void estr_append(echar* dest, char* src) {
	u64 src_len = strlen(src);

	buf_pop(dest); /* to remove the null terminator */
	for (u64 i = 0; i < src_len; ++i) {
		buf_push(dest, src[i]);
	}
	buf_push(dest, '\0');
}

u64 estr_find_last_of(echar* estr, char c) {
	u64 idx = 0;
	for (u64 i = 0; i < buf_len(estr); ++i) {
		if (estr[i] == c) {
			idx = i;
		}
	}
	return idx;
}

echar* estr_sub(echar* estr, u64 start, u64 end) {
	if (end <= start) {
		assert(0);
		return null;
	}

	echar* buf = null;
	for (u64 i = start; i < end; ++i) {
		buf_push(buf, estr[i]);
	}
	buf_push(buf, '\0');
	return buf;
}

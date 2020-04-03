#pragma once

namespace ether {
	struct intern {
		size_t len;
		char* str;
	};

	static arr<intern> interns;

	char* strni(char* start, char* end) {
		size_t len = end - start;
		for (size_t i = 0; i < interns.len; ++i) {
			if (interns[i].len == len && strncmp(interns[i].str, start, len) == 0) {
				return interns[i].str;
			}
		}

		char* str = (char*)malloc(len + 1);
		memcpy(str, start, len);
		str[len] = 0;
		interns.push((intern){ len, str });
		return str;
	}

	char* stri(char* str) {
		return strni(str, str + strlen(str));
	}
}	

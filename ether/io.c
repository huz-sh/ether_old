#include <ether/ether.h>

file ether_read_file(char* fpath) {
	/* TODO: more thorough error checking */
	FILE* fp = fopen(fpath, "r");
	if (!fp) return (file){ };
	fseek(fp, 0, SEEK_END);
	uint size = ftell(fp);
	rewind(fp);
	
	char* contents = (char*)malloc(size + 1);
	fread((void*)contents, sizeof(char), size, fp);
	fclose(fp);
	
	return (file){ fpath, contents, size };
}

char* get_line_at(file f, uint64 l) {
	assert(l != 0);
		
	const uint64 target_newline = l - 1;
	uint64 nnewline = 0;
	char* line_to_print = f.contents;

	while (nnewline < target_newline) {
		while (*line_to_print != '\n') {
			if (*line_to_print == '\0') return null;
			++line_to_print;
		}
		++line_to_print;
		++nnewline;
	}
	return line_to_print;
}

int print_file_line(file f, uint64 l) {
	assert(l != 0);

	char* line_to_print = get_line_at(f, l);
	assert(line_to_print);

	while (*line_to_print != '\n') {
		if (*line_to_print == '\0') break;
		
		if (*line_to_print == '\t') printf("    ");
		else printf("%c", *line_to_print);
		++line_to_print;
	}
	printf("\n");
	return ETHER_SUCCESS;
}

int print_file_line_with_info(file f, uint64 l) {
	printf("%6ld | ", l);
	return print_file_line(f, l);
}

int print_marker_arrow_ln(file f, uint64 l, uint32 c) {
	char* whitespace_start = get_line_at(f, l);
	assert(whitespace_start);
	const char* marker = whitespace_start + c - 1;

	while (whitespace_start != marker) {
		if (*whitespace_start == '\0') return ETHER_ERROR;
		if (*whitespace_start == '\t') printf("    ");
		else printf(" ");
		++whitespace_start;
	}
	printf("^\n");
	return ETHER_SUCCESS;
}

int print_marker_arrow_with_info_ln(file f, uint64 l, uint32 c) {
	printf("%6s | ", "");
	return print_marker_arrow_ln(f, l, c);
}

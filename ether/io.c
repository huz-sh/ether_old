#include <ether/ether.h>

static void print_tab(void);

SourceFile* ether_read_file(char* fpath) {
	/* TODO: more thorough error checking */
	FILE* fp = fopen(fpath, "r");
	if (!fp) return null;
	fseek(fp, 0, SEEK_END);
	uint size = ftell(fp);
	rewind(fp);

	char* contents = (char*)malloc(size + 1);
	fread((void*)contents, sizeof(char), size, fp);
	fclose(fp);

	SourceFile* file = (SourceFile*)malloc(sizeof(SourceFile));
	file->fpath = fpath;
	file->contents = contents;
	file->len = size;
	return file;
}

char* get_line_at(SourceFile* file, u64 line) {
	assert(line != 0);
	assert(file->contents);

	const u64 target_newline = line - 1;
	u64 newline_count = 0;
	char* line_to_print = file->contents;

	while (newline_count < target_newline) {
		while (*line_to_print != '\n') {
			if (*line_to_print == '\0') return null;
			++line_to_print;
		}
		++line_to_print;
		++newline_count;
	}
	return line_to_print;
}

error_code print_file_line(SourceFile* file, u64 line) {
	assert(line != 0);

	char* line_to_print = get_line_at(file, line);
	assert(line_to_print);

	while (*line_to_print != '\n') {
		if (*line_to_print == '\0') break;

		if (*line_to_print == '\t') print_tab();
		else printf("%c", *line_to_print);
		++line_to_print;
	}
	printf("\n");
	return ETHER_SUCCESS;
}

error_code print_file_line_with_info(SourceFile* file, u64 line) {
	printf("%6ld | ", line);
	return print_file_line(file, line);
}

error_code print_marker_arrow_ln(SourceFile* file, u64 line, u32 column) {
	char* whitespace_start = get_line_at(file, line);
	assert(whitespace_start);
	const char* marker = whitespace_start + column - 1;

	while (whitespace_start != marker) {
		if (*whitespace_start == '\0') return ETHER_ERROR;
		if (*whitespace_start == '\t') print_tab();
		else printf(" ");
		++whitespace_start;
	}
	printf("^\n");
	return ETHER_SUCCESS;
}

error_code print_marker_arrow_with_info_ln(SourceFile* file,
										   u64 line, u32 column) {
	printf("%6s | ", "");
	return print_marker_arrow_ln(file, line, column);
}

static void print_tab(void) {
	for (u8 i = 0; i < TAB_SIZE; ++i) {
		printf(" ");
	}
}

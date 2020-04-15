#include <ether/ether.h>

bool is_token_identical(Token* a, Token* b) {
	assert(a);
	assert(b);

	if (str_intern(a->lexeme) == str_intern(b->lexeme)) {
		return true;
	}
	return false;
}

void token_error(bool* error_occured, uint* error_count,
				 Token* token, const char* fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	printf("%s:%ld:%d: error: ",
		   token->srcfile->fpath, token->line, token->column);
	vprintf(fmt, ap);
	va_end(ap);
	printf("\n");

	print_file_line_with_info(token->srcfile, token->line);
	print_marker_arrow_with_info_ln(token->srcfile, token->line, token->column);

	(*error_occured) = ETHER_ERROR;
	++(*error_count);
}

void token_warning(Token* token, const char* fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	printf("%s:%ld:%d: warning: ",
		   token->srcfile->fpath, token->line, token->column);
	vprintf(fmt, ap);
	va_end(ap);
	printf("\n");

	print_file_line_with_info(token->srcfile, token->line);
	print_marker_arrow_with_info_ln(token->srcfile, token->line, token->column);
}

void token_note(Token* token, const char* fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	printf("%s:%ld:%d: note: ",
		   token->srcfile->fpath, token->line, token->column);
	vprintf(fmt, ap);
	va_end(ap);
	printf("\n");

	print_file_line_with_info(token->srcfile, token->line);
	print_marker_arrow_with_info_ln(token->srcfile, token->line, token->column);	
}

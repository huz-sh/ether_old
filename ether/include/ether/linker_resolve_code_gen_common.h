#ifndef __LINKER_RESOLVE_COMMON_H
#define __LINKER_RESOLVE_COMMON_H

#define error(t, s, ...) token_error(&error_occured, &error_count, \
									 t, s, ##__VA_ARGS__)

#define warning(t, s, ...) token_warning(t, s, ##__VA_ARGS__)

#define note(t, s, ...) token_note(t, s, ##__VA_ARGS__);

#endif

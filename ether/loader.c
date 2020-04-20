#include <ether/ether.h>

/* the loader's job is to generate an object file
 * given a source file path */
error_code loader_load(Loader* loader, char* fpath, char* obj_fpath) {
	loader->fpath = fpath;
	loader->obj_fpath = obj_fpath;	
}

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

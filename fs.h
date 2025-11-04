#include "inode.h"

void mkfs(FILE * file);

FILE * fsopen();

void fswrite(FILE * f, uint32_t file);

void write_helper(char * buffer, size_t size, FILE * f, uint32_t file);
 
void fsread(FILE * f, uint32_t file);


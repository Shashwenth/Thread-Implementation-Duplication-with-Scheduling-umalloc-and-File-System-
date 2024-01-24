#include <stdio.h>

//to use umalloc and ufree instead of malloc and free respectively
#define malloc(arg) umalloc( arg, __FILE__, __LINE__ )
#define free(arg) ufree(arg, __FILE__ , __LINE__ )

#define MEM_SIZE 1024*1024*10 //max 10MB memory available
static char init = '0';
static char mem[MEM_SIZE];

//defining the metadata structure for every assigned block
//this will be stored at the start of the chunk of the memory being allocated
typedef struct block_info{
    unsigned int block_size;
    char block_allocated;
}block_info;

static size_t metadata_size = sizeof(block_info);

void* umalloc(size_t size, char*,int);
void ufree(void* ptr, char*, int);

void prettyPrint();

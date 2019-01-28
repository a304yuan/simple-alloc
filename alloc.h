#ifndef ALLOC_H
#define ALLOC_H

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define ALIGN_SIZE 64
#define INIT_HEAP_SIZE 131072 // 128KB

extern void mem_init();
extern void * mem_alloc(size_t size, bool reserved);
extern void * mem_realloc(void *mem, size_t size);
extern void mem_free(void *mem);

#endif

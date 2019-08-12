#ifndef HW_MALLOC_H
#define HW_MALLOC_H
#include <stdlib.h>

void init();
void *hw_malloc(size_t bytes);
int hw_free(unsigned long mem);///(void * mem)
void *get_start_sbrk(void);/////not used
void split(int s, int i);
void *malloc_bin(int i);
void *malloc_mmap(size_t i);
void print_bin(int i);
void print_mmap();
void merge_bin(int i);

#endif

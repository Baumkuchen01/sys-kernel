#ifndef __STDLIB_H__
#define __STDLIB_H__

#define RAND_MAX 0x7fffffff

#include <stddef.h>

int rand(void);
void srand(unsigned seed);
void *malloc(size_t size);
void free(void * ptr);

#endif

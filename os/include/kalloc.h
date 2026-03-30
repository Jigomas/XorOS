#ifndef KALLOC_H
#define KALLOC_H

#include <stdint.h>

#define PAGE_SIZE 4096u

// allocate one page-aligned 4 KiB page; returns NULL if out of memory
void* kalloc(void);

#endif /* KALLOC_H */

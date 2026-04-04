#include "kalloc.h"

#include <stddef.h>

#define KALLOC_PAGES 4u

static uint8_t  heap[KALLOC_PAGES * PAGE_SIZE] __attribute__((aligned(PAGE_SIZE)));
static uint8_t* bump = heap;

void* kalloc(void) {
    if (bump >= heap + sizeof(heap))
        return NULL;
    void* p = bump;
    bump += PAGE_SIZE;
    return p;
}

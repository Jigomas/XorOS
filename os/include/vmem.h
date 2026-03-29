#ifndef VMEM_H
#define VMEM_H

#include <stdint.h>

// PTE flags (Sv32)
#define PTE_V (1u << 0u)  // valid
#define PTE_R (1u << 1u)  // readable
#define PTE_W (1u << 2u)  // writable
#define PTE_X (1u << 3u)  // executable
#define PTE_U (1u << 4u)  // user-accessible

// map one 4KiB page: virt → phys with given flags
void vmem_map(uint32_t virt, uint32_t phys, uint32_t flags);

// enable Sv32 translation (write satp, sfence.vma)
void vmem_enable(void);

#endif /* VMEM_H */

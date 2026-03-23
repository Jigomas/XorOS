#include "vmem.h"

// Sv32 page tables
// all 64KiB addresses have vpn[1]=0, so one L0 table suffices
static uint32_t root_pt[1024] __attribute__((aligned(4096)));
static uint32_t l0_pt[1024]   __attribute__((aligned(4096)));

void vmem_map(uint32_t virt, uint32_t phys, uint32_t flags) {
    uint32_t vpn0   = (virt >> 12u) & 0x3FFu;
    uint32_t ppn    = phys >> 12u;

    // link root entry 0 to l0_pt if not yet set
    if (!(root_pt[0] & PTE_V)) {
        uint32_t l0_ppn = (uint32_t)l0_pt >> 12u;
        root_pt[0] = (l0_ppn << 10u) | PTE_V;
    }

    l0_pt[vpn0] = (ppn << 10u) | flags | PTE_V;
}

void vmem_enable(void) {
    uint32_t ppn  = (uint32_t)root_pt >> 12u;
    uint32_t satp = (1u << 31u) | ppn;
    __asm__ volatile("csrw satp, %0" :: "r"(satp));
    __asm__ volatile(".word 0x12000073" ::: "memory");  // sfence.vma zero, zero
}

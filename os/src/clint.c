#include "clint.h"

static uint32_t g_interval;

void clint_init(uint32_t interval) {
    g_interval = interval;
    clint_mtimecmp_write(clint_mtime() + interval);
    // enable machine timer interrupt (MTIE = bit 7 of MIE CSR)
    uint32_t mtie = (1u << 7);
    __asm__ volatile("csrrs zero, mie, %0" ::"r"(mtie) : "memory");
    // enable global machine interrupts (MIE = bit 3 of mstatus)
    __asm__ volatile("csrsi mstatus, 0x8" ::: "memory");
}

void clint_set_next(void) {
    clint_mtimecmp_write(clint_mtime() + g_interval);
}
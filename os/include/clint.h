#ifndef CLINT_H
#define CLINT_H

#include <stdint.h>

// CLINT MMIO layout (matches simulator)
// placed just above UART (0xF000) in the identity-mapped region
#define CLINT_MTIME_LO    ((volatile uint32_t*) 0xF004u)
#define CLINT_MTIME_HI    ((volatile uint32_t*) 0xF008u)
#define CLINT_MTIMECMP_LO ((volatile uint32_t*) 0xF00Cu)
#define CLINT_MTIMECMP_HI ((volatile uint32_t*) 0xF010u)

// read current mtime safely (double-read hi to avoid 32-bit rollover race)
static inline uint64_t clint_mtime(void) {
    uint32_t hi, lo;
    do {
        hi = *CLINT_MTIME_HI;
        lo = *CLINT_MTIME_LO;
    } while (*CLINT_MTIME_HI != hi);
    return ((uint64_t) hi << 32) | lo;
}

// write mtimecmp safely (set hi=MAX first to avoid spurious interrupt)
static inline void clint_mtimecmp_write(uint64_t val) {
    *CLINT_MTIMECMP_HI = UINT32_MAX;
    *CLINT_MTIMECMP_LO = (uint32_t) val;
    *CLINT_MTIMECMP_HI = (uint32_t) (val >> 32);
}

// init: arm timer for first tick at mtime + interval; enables MTIE and MIE
void clint_init(uint32_t interval);

// call from timer interrupt handler to arm next tick
void clint_set_next(void);

#endif /* CLINT_H */
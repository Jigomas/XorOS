#ifndef TRAP_H
#define TRAP_H

#include <stdint.h>

// called from trap_entry (trap.S) — cause, faulting PC, extra info
void trap_handler(uint32_t mcause, uint32_t mepc, uint32_t mtval);

#endif

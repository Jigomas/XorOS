#ifndef TRAP_H
#define TRAP_H

#include <stdint.h>

// saved caller-saved frame indices (matches trap.S layout)
#define TRAP_FRAME_RA 0  // ra  - sp+0
#define TRAP_FRAME_A0 1  // a0  - sp+4
#define TRAP_FRAME_A7 8  // a7  - sp+32

// handler signature: cause, faulting PC, extra info, pointer to saved caller-saved frame
typedef void (*trap_fn_t)(uint32_t mcause, uint32_t mepc, uint32_t mtval, uint32_t* frame);

// register handler for exception (mcause[31]=0) or interrupt (mcause[31]=1)
void trap_register_exc(uint32_t cause, trap_fn_t fn);
void trap_register_int(uint32_t cause, trap_fn_t fn);

// called from trap.S - dispatches via table, falls back to panic
void trap_handler(uint32_t mcause, uint32_t mepc, uint32_t mtval, uint32_t* frame);

#endif /* TRAP_H */

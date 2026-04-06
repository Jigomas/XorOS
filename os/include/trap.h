#ifndef TRAP_H
#define TRAP_H

#include <stdint.h>

// full 32-register frame indices (matches trap.S layout, word offsets)
#define TRAP_FRAME_RA 0   // x1  ra   - sp+0
#define TRAP_FRAME_SP 1   // x2  sp   - sp+4  (original sp before trap)
#define TRAP_FRAME_A0 9   // x10 a0   - sp+36
#define TRAP_FRAME_A7 16  // x17 a7   - sp+64

// handler signature: cause, faulting PC, extra info, pointer to saved caller-saved frame
typedef void (*trap_fn_t)(uint32_t mcause, uint32_t mepc, uint32_t mtval, uint32_t* frame);

// register handler for exception (mcause[31]=0) or interrupt (mcause[31]=1)
void trap_register_exc(uint32_t cause, trap_fn_t fn);
void trap_register_int(uint32_t cause, trap_fn_t fn);

// called from trap.S - dispatches via table, falls back to panic
void trap_handler(uint32_t mcause, uint32_t mepc, uint32_t mtval, uint32_t* frame);

#endif /* TRAP_H */

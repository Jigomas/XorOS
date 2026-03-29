#ifndef TRAP_H
#define TRAP_H

#include <stdint.h>

// handler signature: cause code, faulting PC, extra info (addr/insn)
typedef void (*trap_fn_t)(uint32_t mcause, uint32_t mepc, uint32_t mtval);

// register handler for exception (mcause[31]=0) or interrupt (mcause[31]=1)
void trap_register_exc(uint32_t cause, trap_fn_t fn);
void trap_register_int(uint32_t cause, trap_fn_t fn);

// called from trap.S — dispatches via table, falls back to panic
void trap_handler(uint32_t mcause, uint32_t mepc, uint32_t mtval);

#endif /* TRAP_H */

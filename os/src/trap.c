#include "trap.h"
#include "csr.h"
#include "ecall.h"

static void puts_trap(const char *s) {
    while (*s) sys_putchar(*s++);
}

static void puthex_trap(uint32_t v) {
    puts_trap("0x");
    for (int i = 7; i >= 0; --i) {
        uint8_t n = (uint8_t)((v >> (i * 4)) & 0xF);
        sys_putchar(n < 10 ? (char)('0' + n) : (char)('a' + n - 10));
    }
}

static const char *cause_name(uint32_t mcause) {
    if (mcause & MCAUSE_INT) {
        switch (mcause & ~MCAUSE_INT) {
            case 3:  return "M-mode software interrupt";
            case 7:  return "M-mode timer interrupt";
            case 11: return "M-mode external interrupt";
            default: return "unknown interrupt";
        }
    }
    switch ((csr_exc_t)mcause) {
        case CAUSE_INSN_MISALIGN:  return "instruction address misaligned";
        case CAUSE_INSN_FAULT:     return "instruction access fault";
        case CAUSE_ILLEGAL_INSN:   return "illegal instruction";
        case CAUSE_BREAKPOINT:     return "breakpoint";
        case CAUSE_LOAD_MISALIGN:  return "load address misaligned";
        case CAUSE_LOAD_FAULT:     return "load access fault";
        case CAUSE_STORE_MISALIGN: return "store address misaligned";
        case CAUSE_STORE_FAULT:    return "store access fault";
        case CAUSE_ECALL_U:        return "ecall from U-mode";
        case CAUSE_ECALL_M:        return "ecall from M-mode";
        default:                   return "unknown exception";
    }
}

void trap_handler(uint32_t mcause, uint32_t mepc, uint32_t mtval) {
    puts_trap("[trap] ");
    puts_trap(cause_name(mcause));
    puts_trap(" at ");
    puthex_trap(mepc);
    if (mtval) {
        puts_trap(" (");
        puthex_trap(mtval);
        puts_trap(")");
    }
    puts_trap("\n");
    sys_exit();
}

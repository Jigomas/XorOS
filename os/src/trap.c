#include "trap.h"

#include "csr.h"
#include "ecall.h"

#define MAX_CAUSES 16

static trap_fn_t exc_table[MAX_CAUSES];  // exception handlers (mcause[31]=0)
static trap_fn_t int_table[MAX_CAUSES];  // interrupt handlers (mcause[31]=1)

static void puts_trap(const char* s) {
    while (*s)
        sys_putchar(*s++);
}

static void puthex_trap(uint32_t v) {
    puts_trap("0x");
    for (int i = 7; i >= 0; --i) {
        uint8_t n = (uint8_t) ((v >> (i * 4)) & 0xF);
        sys_putchar(n < 10 ? (char) ('0' + n) : (char) ('a' + n - 10));
    }
}

static const char* cause_name(uint32_t mcause) {
    if (mcause & MCAUSE_INT) {
        switch (mcause & ~MCAUSE_INT) {
            case 3:
                return "M-mode software interrupt";
            case 7:
                return "M-mode timer interrupt";
            case 11:
                return "M-mode external interrupt";
            default:
                return "unknown interrupt";
        }
    }
    switch ((csr_exc_t) mcause) {
        case CAUSE_INSN_MISALIGN:
            return "instruction address misaligned";
        case CAUSE_INSN_FAULT:
            return "instruction access fault";
        case CAUSE_ILLEGAL_INSN:
            return "illegal instruction";
        case CAUSE_BREAKPOINT:
            return "breakpoint";
        case CAUSE_LOAD_MISALIGN:
            return "load address misaligned";
        case CAUSE_LOAD_FAULT:
            return "load access fault";
        case CAUSE_STORE_MISALIGN:
            return "store address misaligned";
        case CAUSE_STORE_FAULT:
            return "store access fault";
        case CAUSE_ECALL_U:
            return "ecall from U-mode";
        case CAUSE_ECALL_S:
            return "ecall from S-mode";
        case CAUSE_ECALL_M:
            return "ecall from M-mode";
        case CAUSE_INSN_PAGE_FAULT:
            return "instruction page fault";
        case CAUSE_LOAD_PAGE_FAULT:
            return "load page fault";
        case CAUSE_STORE_PAGE_FAULT:
            return "store page fault";
        default:
            return "unknown exception";
    }
}

static void panic_handler(uint32_t mcause, uint32_t mepc, uint32_t mtval) {
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

// provide your own trap handlers
void trap_register_exc(uint32_t cause, trap_fn_t fn) {
    if (cause < MAX_CAUSES)
        exc_table[cause] = fn;
}

void trap_register_int(uint32_t cause, trap_fn_t fn) {
    if (cause < MAX_CAUSES)
        int_table[cause] = fn;
}

void trap_handler(uint32_t mcause, uint32_t mepc, uint32_t mtval) {
    uint32_t  is_int = mcause & MCAUSE_INT;
    uint32_t  code   = mcause & ~MCAUSE_INT;
    trap_fn_t fn     = (code < MAX_CAUSES) ? (is_int ? int_table[code] : exc_table[code]) : 0;

    if (fn)
        fn(mcause, mepc, mtval);
    else
        panic_handler(mcause, mepc, mtval);
}

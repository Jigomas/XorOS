#pragma once

#include <stdint.h>

// CSR addresses (rv32i machine-level)
typedef enum {
    CSR_MSTATUS  = 0x300,  // machine status
    CSR_MIE      = 0x304,  // machine interrupt enable
    CSR_MTVEC    = 0x305,  // trap-handler base address
    CSR_MSCRATCH = 0x340,  // trap scratch
    CSR_MEPC     = 0x341,  // exception PC
    CSR_MCAUSE   = 0x342,  // trap cause
    CSR_MTVAL    = 0x343,  // bad addr or instruction
    CSR_MIP      = 0x344,  // machine interrupt pending
    CSR_MHARTID  = 0xF14,  // hart id, always 0
} csr_addr_t;

// mstatus bits
#define MSTATUS_MIE  (1u << 3)   // global machine interrupt enable
#define MSTATUS_MPIE (1u << 7)   // saved MIE before trap
#define MSTATUS_MPP  (3u << 11)  // previous privilege mode (2 bits)

// mcause: bit 31 = 1 - interrupt, 0 - exception
#define MCAUSE_INT (1u << 31)

// exception codes
typedef enum {
    CAUSE_INSN_MISALIGN  = 0,   // instruction address misaligned
    CAUSE_INSN_FAULT     = 1,   // instruction access fault
    CAUSE_ILLEGAL_INSN   = 2,   // illegal instruction
    CAUSE_BREAKPOINT     = 3,   // breakpoint
    CAUSE_LOAD_MISALIGN  = 4,   // load address misaligned
    CAUSE_LOAD_FAULT     = 5,   // load access fault
    CAUSE_STORE_MISALIGN = 6,   // store address misaligned
    CAUSE_STORE_FAULT    = 7,   // store access fault
    CAUSE_ECALL_U        = 8,   // ecall from U-mode
    CAUSE_ECALL_M        = 11,  // ecall from M-mode
} csr_exc_t;

// interrupt codes (mcause bit 31 = 1)
#define CAUSE_INT_SW_M    (MCAUSE_INT | 3u)   // machine software interrupt
#define CAUSE_INT_TIMER_M (MCAUSE_INT | 7u)   // machine timer interrupt
#define CAUSE_INT_EXT_M   (MCAUSE_INT | 11u)  // machine external interrupt

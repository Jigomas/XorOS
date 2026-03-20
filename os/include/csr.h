#pragma once

#include <stdint.h>

// CSR addresses (rv32i Machine-level)
#define CSR_MSTATUS  0x300  // machine status
#define CSR_MIE      0x304  // machine interrupt enable
#define CSR_MTVEC    0x305  // trap-handler base address
#define CSR_MSCRATCH 0x340  // scratch register for trap handler
#define CSR_MEPC     0x341  // exception program counter
#define CSR_MCAUSE   0x342  // trap cause
#define CSR_MTVAL    0x343  // trap value (bad address or instruction)
#define CSR_MIP      0x344  // machine interrupt pending
#define CSR_MHARTID  0xF14  // hardware thread id (always 0 on single-core)

// mstatus bits
#define MSTATUS_MIE  (1u << 3)   // global machine interrupt enable
#define MSTATUS_MPIE (1u << 7)   // saved MIE before trap
#define MSTATUS_MPP  (3u << 11)  // previous privilege mode (2 bits)

// mcause: bit 31 = 1 means interrupt, 0 means exception
#define MCAUSE_INT (1u << 31)

// exception codes (mcause bit 31 = 0)
#define CAUSE_INSN_MISALIGN  0u   // instruction address misaligned
#define CAUSE_INSN_FAULT     1u   // instruction access fault
#define CAUSE_ILLEGAL_INSN   2u   // illegal instruction
#define CAUSE_BREAKPOINT     3u   // breakpoint
#define CAUSE_LOAD_MISALIGN  4u   // load address misaligned
#define CAUSE_LOAD_FAULT     5u   // load access fault
#define CAUSE_STORE_MISALIGN 6u   // store address misaligned
#define CAUSE_STORE_FAULT    7u   // store access fault
#define CAUSE_ECALL_U        8u   // ecall from U-mode
#define CAUSE_ECALL_M        11u  // ecall from M-mode

// interrupt codes (mcause bit 31 = 1)
#define CAUSE_INT_SW_M    (MCAUSE_INT | 3u)   // machine software interrupt
#define CAUSE_INT_TIMER_M (MCAUSE_INT | 7u)   // machine timer interrupt
#define CAUSE_INT_EXT_M   (MCAUSE_INT | 11u)  // machine external interrupt

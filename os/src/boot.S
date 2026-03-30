.section .text.boot
.align 2
.global _start

_start:
    li    sp, 0x2000
    .option push
    .option norelax
    la    gp, __global_pointer$
    .option pop
    la    t0, trap_entry
    csrw  mtvec, t0
    li    t0, (1 << 11)    # mstatus.MPP = S-mode (01)
    csrw  mstatus, t0
    la    t0, kernel_main
    csrw  mepc, t0
    mret

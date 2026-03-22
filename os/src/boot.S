.section .text.boot
.global _start

_start:
    li    sp, 0x2000
    .option push
    .option norelax
    la    gp, __global_pointer$
    .option pop
    la    t0, trap_entry
    csrw  mtvec, t0
    call  kernel_main
1:  j     1b

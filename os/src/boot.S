.section .text.boot
.global _start

_start:
    li    sp, 0x2000
    la    t0, trap_entry
    csrw  mtvec, t0
    call  kernel_main
1:  j     1b

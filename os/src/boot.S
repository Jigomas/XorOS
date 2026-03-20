.section .text.boot
.global _start

_start:
    li    sp, 0x2000
    call  kernel_main
1:  j     1b

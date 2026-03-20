#ifndef XOROS_ECALL_H
#define XOROS_ECALL_H

#define SYS_PUTCHAR 1
#define SYS_EXIT    10

static inline void sys_putchar(char c) {
    register int a7 __asm__("a7") = SYS_PUTCHAR;
    register int a0 __asm__("a0") = c;
    __asm__ volatile ("ecall" : "+r"(a0) : "r"(a7));
}

static inline void sys_exit(void) {
    register int a7 __asm__("a7") = SYS_EXIT;
    __asm__ volatile ("ecall" :: "r"(a7));
}

#endif

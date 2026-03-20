#include "ecall.h"

static void print(const char* s) {
    while (*s)
        sys_putchar(*s++);
}

void kernel_main(void) {
    print("Hello from XorOS!\n");
    sys_exit();
}

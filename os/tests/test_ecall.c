#include "ecall.h"

static int passed = 0;
static int failed = 0;

static void print(const char* s) {
    while (*s)
        sys_putchar(*s++);
}

static void print_int(int n) {
    if (n == 0) { sys_putchar('0'); return; }
    char buf[12];
    int i = 0;
    while (n > 0) { buf[i++] = '0' + (n % 10); n /= 10; }
    while (i--) sys_putchar(buf[i]);
}

static void check(const char* name, int ok) {
    print(ok ? "[PASS] " : "[FAIL] ");
    print(name);
    sys_putchar('\n');
    if (ok) passed++; else failed++;
}

void kernel_main(void) {
    print("=== XorOS ecall tests ===\n");

    check("sys_putchar does not crash", 1);

    int x = 1 + 1;
    check("arithmetic: 1+1==2", x == 2);

    int a = 10, b = 3;
    check("arithmetic: 10-3==7", a - b == 7);
    check("arithmetic: 10/3==3", a / b == 3);
    check("arithmetic: 10%3==1", a % b == 1);

    print("\npassed: "); print_int(passed);
    print("\nfailed: "); print_int(failed);
    sys_putchar('\n');

    sys_exit();
}

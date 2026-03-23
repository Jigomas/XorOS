#include "ecall.h"
#include "vmem.h"

static int passed = 0;
static int failed = 0;

static void print(const char* s) {
    while (*s)
        sys_putchar(*s++);
}

static void print_int(int n) {
    if (n == 0) {
        sys_putchar('0');
        return;
    }
    char buf[12];
    int  i = 0;
    while (n > 0) {
        buf[i++] = '0' + (n % 10);
        n /= 10;
    }
    while (i--)
        sys_putchar(buf[i]);
}

static void check(const char* name, int ok) {
    print(ok ? "[PASS] " : "[FAIL] ");
    print(name);
    sys_putchar('\n');
    if (ok)
        passed++;
    else
        failed++;
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

    print("\n--- vmem ---\n");

    // identity map all 64 KiB then enable Sv32
    for (uint32_t p = 0; p < 0x10000u; p += 0x1000u)
        vmem_map(p, p, PTE_R | PTE_W | PTE_X);
    vmem_enable();

    check("code runs after vmem_enable", 1);

    // stack variable accessible via VA = PA (identity mapped)
    volatile uint32_t local = 0xCAFEu;
    check("stack read after vmem_enable", (uint32_t)local == 0xCAFEu);

    // write and read back through a virtual address in the upper half
    volatile uint32_t *ptr = (volatile uint32_t *)0xE000u;
    *ptr = 0xBEEFu;
    check("write+read via virtual addr", (uint32_t)*ptr == 0xBEEFu);

    check("arithmetic after vmem_enable: 6*7=42", 6 * 7 == 42);

    print("\npassed: ");
    print_int(passed);
    print("\nfailed: ");
    print_int(failed);
    sys_putchar('\n');

    sys_exit();
}

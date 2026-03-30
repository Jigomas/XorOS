#include <stddef.h>

#include "ecall.h"
#include "kalloc.h"
#include "spinlock.h"
#include "pipe.h"
#include "process.h"
#include "scheduler.h"
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

static void noop(void) {}

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
    check("stack read after vmem_enable", (uint32_t) local == 0xCAFEu);

    // write and read back through a virtual address in the upper half
    volatile uint32_t* ptr = (volatile uint32_t*) 0xE000u;
    *ptr                   = 0xBEEFu;
    check("write+read via virtual addr", (uint32_t) *ptr == 0xBEEFu);

    check("arithmetic after vmem_enable: 6*7=42", 6 * 7 == 42);

    print("\n--- canary ---\n");

    check("STACK_CANARY == 0xDEADBEEF", STACK_CANARY == 0xDEADBEEFu);

    sched_init();
    int pid = sched_spawn(noop);

    int slot = -1;
    for (int i = 0; i < MAX_PROCS; i++) {
        if ((int) procs[i].pid == pid) {
            slot = i;
            break;
        }
    }

    check("canary set on spawn", procs[slot].stack.canary == STACK_CANARY);

    sched_yield();

    check("canary intact after normal exit", procs[slot].stack.canary == STACK_CANARY);

    print("\n--- pipe ---\n");

    pipe_t p;
    pipe_init(&p);
    check("pipe_init: count == 0", p.count == 0);

    check("pipe_write returns 0", pipe_write(&p, 'X') == 0);

    uint8_t c = 0;
    check("pipe_read returns 0", pipe_read(&p, &c) == 0);
    check("pipe_read correct byte", c == 'X');

    check("pipe_read empty returns -1", pipe_read(&p, &c) == -1);

    for (int i = 0; i < PIPE_SIZE; i++)
        pipe_write(&p, (uint8_t) i);
    check("pipe_write full returns -1", pipe_write(&p, 0) == -1);

    for (int i = 0; i < PIPE_SIZE; i++)
        pipe_read(&p, &c);
    check("pipe wrap-around write ok", pipe_write(&p, 'W') == 0);
    check("pipe wrap-around read ok", pipe_read(&p, &c) == 0 && c == 'W');

    print("\n--- kalloc ---\n");

    void* pg1 = kalloc();
    check("kalloc returns non-NULL", pg1 != NULL);
    check("kalloc page-aligned", ((uint32_t) pg1 % PAGE_SIZE) == 0);

    void* pg2 = kalloc();
    check("two allocs differ", pg1 != pg2);
    check("pages are PAGE_SIZE apart", (uint32_t) pg2 - (uint32_t) pg1 == PAGE_SIZE);

    print("\n--- spinlock ---\n");

    spinlock_t sl = SPINLOCK_INIT;
    check("spinlock_init: unlocked", sl.locked == 0);
    spinlock_lock(&sl);
    check("spinlock_lock: locked", sl.locked == 1);
    spinlock_unlock(&sl);
    check("spinlock_unlock: unlocked", sl.locked == 0);

    print("\npassed: ");
    print_int(passed);
    print("\nfailed: ");
    print_int(failed);
    sys_putchar('\n');

    sys_exit();
}

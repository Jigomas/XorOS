#include "ecall.h"
#include "scheduler.h"
#include "vmem.h"

static void print(const char *s) {
    while (*s)
        sys_putchar(*s++);
}

static void task_a(void) {
    print("task A: hello\n");
    sched_yield();
    print("task A: bye\n");
}

static void task_b(void) {
    print("task B: hello\n");
    sched_yield();
    print("task B: bye\n");
}

void kernel_main(void) {
    print("Hello from XorOS!\n");

    // identity map all 64KiB before enabling paging
    for (uint32_t p = 0; p < 0x10000u; p += 0x1000u)
        vmem_map(p, p, PTE_R | PTE_W | PTE_X);
    vmem_enable();

    sched_init();
    sched_spawn(task_a);
    sched_spawn(task_b);
    // yield until all spawned processes finish 
    sched_yield();
    sched_yield();
    print("kernel: all done\n");
    sys_exit();
}

#include "clint.h"
#include "ecall.h"
#include "pipe.h"
#include "scheduler.h"
#include "trap.h"
#include "uart.h"
#include "vmem.h"

static pipe_t chan;

static void task_a(void) {
    const char* msg = "hi\n";
    for (int i = 0; msg[i]; i++)
        pipe_write(&chan, (uint8_t) msg[i]);
    sched_yield();
}

static void task_b(void) {
    uint8_t c;
    while (pipe_read(&chan, &c) == 0)
        uart_putchar((char) c);
    sched_yield();
}

static void timer_handler(uint32_t mcause, uint32_t mepc, uint32_t mtval, uint32_t* frame) {
    (void) mcause;
    (void) mepc;
    (void) mtval;
    (void) frame;
    clint_set_next();
    sched_yield();
}

void kernel_main(void) {
    uart_puts("Hello from XorOS!\n");

    // identity map all 64KiB before enabling paging
    for (uint32_t p = 0; p < 0x10000u; p += 0x1000u)
        vmem_map(p, p, PTE_R | PTE_W | PTE_X);
    vmem_enable();

    trap_register_int(7, timer_handler);
    clint_init(1000);  // timer every 1000 instructions

    pipe_init(&chan);
    sched_init();
    sched_spawn(task_a);
    sched_spawn(task_b);
    sched_yield();
    sched_yield();
    uart_puts("kernel: all done\n");
    sys_exit();
}

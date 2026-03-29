#include "scheduler.h"

#include "ecall.h"

static void print_str(const char* s) {
    for (; *s; ++s)
        sys_putchar(*s);
}

static void print_uint(uint32_t n) {
    if (n >= 10)
        print_uint(n / 10);
    sys_putchar('0' + (char) (n % 10));
}

static void check_canary(int idx) {
    if (procs[idx].stack.canary != STACK_CANARY) {
        print_str("panic: stack overflow in pid ");
        print_uint(procs[idx].pid);
        sys_putchar('\n');
        sys_exit();
    }
}

proc_t procs[MAX_PROCS];

static uint32_t next_pid = 1;
static int      current  = 0;

// trampoline: first function a new process runs after context switch.
// calls entry(), then exits - avoids ra confusion in context_switch
static void proc_trampoline(void) {
    procs[current].entry();
    sched_exit();
}

void sched_init(void) {
    for (int i = 0; i < MAX_PROCS; i++)
        procs[i].state = PROC_UNUSED;

    procs[0].state        = PROC_RUNNING;
    procs[0].pid          = 0;
    procs[0].stack.canary = STACK_CANARY;
}

int sched_spawn(void (*entry)(void)) {
    for (int i = 0; i < MAX_PROCS; i++) {
        if (procs[i].state != PROC_UNUSED)
            continue;

        procs[i].state = PROC_READY;
        procs[i].pid   = next_pid++;
        procs[i].entry = entry;

        procs[i].stack.canary = STACK_CANARY;
        procs[i].ctx.ra       = (uint32_t) proc_trampoline;
        procs[i].ctx.sp       = (uint32_t) (procs[i].stack.data + sizeof(procs[i].stack.data));

        return (int) procs[i].pid;
    }
    return -1;
}
// round - robin planner in search of next ready process
void sched_yield(void) {
    int old = current;
    check_canary(old);

    for (int i = 1; i <= MAX_PROCS; i++) {
        int next = (old + i) % MAX_PROCS;
        if (procs[next].state == PROC_READY) {
            if (procs[old].state == PROC_RUNNING)
                procs[old].state = PROC_READY;
            procs[next].state = PROC_RUNNING;
            current           = next;
            context_switch(&procs[old].ctx, &procs[next].ctx);
            return;
        }
    }
}

void sched_exit(void) {
    check_canary(current);
    procs[current].state = PROC_ZOMBIE;

    for (int i = 1; i <= MAX_PROCS; i++) {
        int next = (current + i) % MAX_PROCS;
        if (procs[next].state == PROC_READY) {
            int old           = current;
            procs[next].state = PROC_RUNNING;
            current           = next;
            context_switch(&procs[old].ctx, &procs[next].ctx);
            return;
        }
    }
    sys_exit();
}

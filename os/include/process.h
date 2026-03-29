#ifndef PROCESS_H
#define PROCESS_H

#include <stdint.h>

#define MAX_PROCS    4            // max concurrent processes
#define STACK_SIZE   512          // stack per process, bytes
#define STACK_CANARY 0xDEADBEEFu  // written at bottom; overflow overwrites it

typedef enum {
    PROC_UNUSED  = 0,
    PROC_READY   = 1,
    PROC_RUNNING = 2,
    PROC_ZOMBIE  = 3,
} proc_state_t;

// callee-saved regs + ra, saved on context switch
typedef struct {
    uint32_t ra;
    uint32_t sp;
    uint32_t s0;
    uint32_t s1;
    uint32_t s2;
    uint32_t s3;
    uint32_t s4;
    uint32_t s5;
    uint32_t s6;
    uint32_t s7;
    uint32_t s8;
    uint32_t s9;
    uint32_t s10;
    uint32_t s11;
} context_t;

// stack with canary at the bottom; stack grows down from top
typedef struct {
    uint32_t canary;                               // must equal STACK_CANARY
    uint8_t  data[STACK_SIZE - sizeof(uint32_t)];  // usable stack space
} proc_stack_t;

// process control block
typedef struct {
    proc_state_t state;
    uint32_t     pid;
    context_t    ctx;
    void (*entry)(void);  // entry function, called by trampoline
    proc_stack_t stack;   // grows down; canary at lowest address
} proc_t;

#endif /* PROCESS_H */

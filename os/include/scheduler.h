#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "process.h"

// global process table 
extern proc_t procs[MAX_PROCS];

// initialize scheduler, set up idle process 
void sched_init(void);

// create process with entry function, returns pid or -1 on failure 
int sched_spawn(void (*entry)(void));

// give up cpu - switch to next ready process 
void sched_yield(void);

// mark current process as zombie and yield 
void sched_exit(void);

// context switch: save current ctx, restore next ctx (sched_switch.S)
void context_switch(context_t *old, context_t *new);

#endif

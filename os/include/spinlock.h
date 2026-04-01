#ifndef SPINLOCK_H
#define SPINLOCK_H

#include <stdint.h>

typedef struct {
    uint32_t locked;
} spinlock_t;

#define SPINLOCK_INIT {0}

static inline void spinlock_aquire(spinlock_t* s) {
    uint32_t tmp;
    uint32_t one = 1;
    do {
        __asm__ volatile(
            "amoswap.w.aq %0, %1, (%2)"
            : "=&r"(tmp)
            : "r"(one), "r"(&s->locked)
            : "memory");
    } while (tmp);
}

static inline void spinlock_release(spinlock_t* s) {
    __asm__ volatile(
        "amoswap.w.rl x0, x0, (%0)" ::"r"(&s->locked) : "memory");
}

#endif /* SPINLOCK_H */

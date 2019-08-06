#ifndef _RPI_SYNCH_H
#define _RPI_SYNCH_H

#include <stdint.h>

typedef uint32_t SpinLock_t;
/**
 * Currently mutex is a reentrant spin lock.
 */
typedef struct Mutex_s {
    SpinLock_t xSpinLock;
    // Process ID, must be unique to  each process.
    // If it is -1 then this indicates no process currently holds the lock
    uint32_t ulProcId;
    uint32_t ulNestedCount;
} Mutex_t;

// For simplicity, use this to initialize the mutex
#define MUTEX_INIT_VAL {0, -1, 0}

extern void spinlock_take(uint32_t *lock);
extern void spinlock_give(uint32_t *lock);

void mutex_init(Mutex_t *xMutex);
void mutex_take(Mutex_t *xMutex);
void mutex_give(Mutex_t *xMutex);

#endif /* _RPI_SYNCH_H */

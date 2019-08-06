#include "Drivers/rpi_synch.h"
#include "Drivers/rpi_core.h"

void mutex_init(Mutex_t *xMutex) {
    xMutex->xSpinLock = 0;
    xMutex->ulProcId = -1;    
    xMutex->ulNestedCount = 0;
}

void mutex_take(Mutex_t *xMutex) {
    if (xMutex->ulProcId == CPUID()) {
        xMutex->ulNestedCount++;
        return;
    }
    spinlock_take(&xMutex->xSpinLock);
    xMutex->ulProcId = CPUID();
    xMutex->ulNestedCount = 1;
}

void mutex_give(Mutex_t *xMutex) {
    if (xMutex->ulProcId != CPUID()) {
        // Should assert here.
        return;
    }
    xMutex->ulNestedCount--;
    if (xMutex->ulNestedCount == 0) {
        xMutex->ulProcId = -1;
        spinlock_give(&xMutex->xSpinLock);
    }
}

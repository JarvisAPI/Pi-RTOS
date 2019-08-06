#include <syncqueue.h>
#include <printk.h>

uint32_t vSyncQueueInit(SyncQueue_t *xQueue, uint32_t ulMaxSize) {
    xQueue->ulHead = 0;
    xQueue->ulSize = 0;
    xQueue->ulMaxSize = ulMaxSize;
    xQueue->xLock = 0;
    
    xQueue->xItems = (void **) pvPortMalloc( sizeof(void *) * ulMaxSize );

    return xQueue->xItems == NULL;
}

void vSyncQueueCleanup(SyncQueue_t *xQueue) {
    vPortFree(xQueue->xItems);
}

void vSyncQueueEnqueue(SyncQueue_t *xQueue, void *xItem) {
    portSPIN_LOCK_ACQUIRE( &xQueue->xLock );
    if (xQueue->ulSize >= xQueue->ulMaxSize) {
        portSPIN_LOCK_RELEASE( &xQueue->xLock );
        // Relase lock before displaying error, so we don't block
        // other processes when we are waiting for the spinlock for print.
        printk("No More Space in Sync Queue! Enqueue Ignored\r\n");
        return;
    }
    uint32_t ulTail = (xQueue->ulHead + xQueue->ulSize) % xQueue->ulMaxSize;
    xQueue->xItems[ulTail] = xItem;
    xQueue->ulSize++;
    portSPIN_LOCK_RELEASE( &xQueue->xLock );
}

void * vSyncQueueDequeue(SyncQueue_t *xQueue) {
    void *xItem;
    portSPIN_LOCK_ACQUIRE( &xQueue->xLock );
    if (xQueue->ulSize != 0) {
        xItem = xQueue->xItems[xQueue->ulHead];
        xQueue->ulHead = (xQueue->ulHead + 1) % xQueue->ulMaxSize;
        xQueue->ulSize--;
    }
    portSPIN_LOCK_RELEASE( &xQueue->xLock );
    return xItem;
}

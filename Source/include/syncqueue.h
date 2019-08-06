#ifndef _SYNC_QUEUE_H_
#define _SYNC_QUEUE_H_

#include "FreeRTOS.h"

typedef struct SyncQueue_s {
    void ** xItems;
    uint32_t ulHead;
    uint32_t ulSize;
    uint32_t ulMaxSize;
    SpinLock_t xLock;
} SyncQueue_t;

/**
 * @param ulMaxSize: more efficient if it is a power of 2.
 * @return 0 on success, 1 on failure.
 */
uint32_t vSyncQueueInit(SyncQueue_t *xQueue, uint32_t ulMaxSize);
void vSyncQueueCleanup(SyncQueue_t *xQueue);
void vSyncQueueEnqueue(SyncQueue_t *xQueue, void *xItem);
void * vSyncQueueDequeue(SyncQueue_t *xQueue);

#endif /* _SYNC_QUEUE_H_ */

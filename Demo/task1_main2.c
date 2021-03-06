#ifdef TASK1B
#include <FreeRTOS.h>
#include <task.h>
#include <printk.h>

#include "Drivers/rpi_gpio.h"
#include "Drivers/rpi_irq.h"
#include "Drivers/rpi_aux.h"

typedef struct TaskInfo_s
{
    int iTaskNumber;
    TickType_t xWCET;
    TickType_t xPeriod;
    TickType_t xRelativeDeadline;
    const char* name;
    const char* color;
} TaskInfo_t;


// Define tasks
const int iNumTasks = 2;
TaskInfo_t tasks[] =
{
    {1, 900, 4000, 7000, "Task 1", "[32;1m"},
    {2, 10, 100, 100, "Task 2", "[32;1m"}
};

const int iNumAsyncs = 1;
TaskInfo_t asyncs[] = 
{
    {1, 100, 4000, 4000, "ASYNC", "[32;2m"}
};

typedef void (*vCB_t(int, void(*)(int)))(int);

// TODO Setup job fifo
vCB_t* pvAperiodicRequests[1];
int iNumAperiodicRequests = 0;

void AsyncJob(void *pParam);
// Task never delays itself, always syspends itself
TaskHandle_t server;
//TODO Have a FIFI of tasks
void CBSServer(void* pParam)
{
    printk("SERVER STARTED\r\n");
    while(1)
    {
        vServerCBSRunJob();
    }
}

void TimingTestTask(void *pParam) {
    while(1) {
        TaskInfo_t* xTaskInfo = (TaskInfo_t*) pParam;
        
        printk("\033%s[ Task %d ] Started at %u\r\n\033[0m", xTaskInfo->color,
               xTaskInfo->iTaskNumber,
               xTaskGetTickCount());
        
        vBusyWait(xTaskInfo->xWCET);
        
        printk("\033%s[ Task %d ] Ended at %u\r\n\033[0m", xTaskInfo->color,
               xTaskInfo->iTaskNumber,
               xTaskGetTickCount());
        vEndTaskPeriod();
    }
}

const char* pcDebug = "Hello World!";

void AsyncTestTask(void *pParam)
{
        TaskInfo_t* xTaskInfo = (TaskInfo_t*) pParam;
        vEndTaskPeriod();

        printk("\033%s[ Async Task %d ] at %u\r\n\033[0m", xTaskInfo->color,
                                                           xTaskInfo->iTaskNumber,
                                                           xTaskGetTickCount());
        vServerCBSNotify(server, (TaskFunction_t) AsyncJob, (void *) pcDebug);
        vTaskDelete( NULL );
}

void AsyncJob(void *pParam)
{
    char *pcString = (char *) pParam;
    printk("Async Job obtained: %s\r\n", pcString);
    printk("Busy wait for fun!\r\n");
    vBusyWait(10);
    printk("Done busy waiting...\r\n");
}

void ListenerTask(void *pParam) {
    while(1) {
        TaskInfo_t* xTaskInfo = (TaskInfo_t*) pParam;
        
        printk("\033%s[ Task %d ] Started at %u\r\n\033[0m", xTaskInfo->color,
               xTaskInfo->iTaskNumber,
               xTaskGetTickCount());
        
        vBusyWait(xTaskInfo->xWCET);
        
        printk("\033%s[ Task %d ] Ended at %u\r\n\033[0m", xTaskInfo->color,
               xTaskInfo->iTaskNumber,
               xTaskGetTickCount());
        
        for (int i = 0; i < 10; i++) {
            char *ch = NULL;
            int read = 0;
            read = rpi_aux_getc_nonblocking(&ch);
            if (read) {
                xTaskCreate(AsyncTestTask, asyncs[0].name, 256, (void *) &asyncs[0],
                            2, asyncs[0].xWCET, asyncs[0].xRelativeDeadline,
                            asyncs[0].xPeriod, NULL);
            }
        }
        
        vEndTaskPeriod();
    }
}

/**
 *	This is the systems main entry, some call it a boot thread.
 *
 *	-- Absolutely nothing wrong with this being called main(), just it doesn't have
 *	-- the same prototype as you'd see in a linux program.
 **/
int main(void) {
    rpi_cpu_irq_disable();
    rpi_aux_mu_init();

    // Create tasks
    xTaskCreate(TimingTestTask, tasks[0].name, 256, (void *) &tasks[0],
                PRIORITY_EDF, tasks[0].xWCET, tasks[0].xRelativeDeadline,
                tasks[0].xPeriod, NULL);
    xTaskCreate(ListenerTask, tasks[1].name, 256, (void *) &tasks[1],
                PRIORITY_EDF, tasks[1].xWCET, tasks[1].xRelativeDeadline,
                tasks[1].xPeriod, NULL);

    // Create the server:
    xServerCBSCreate(CBSServer, "CBS Server", 256, NULL, PRIORITY_EDF, 2000, 3000, &server);
    vPrintSchedule();
    vVerifyLLBound();

    vTaskStartScheduler();

    /*
     *	We should never get here, but just in case something goes wrong,
     *	we'll place the CPU into a safe loop.
     */
    while(1) {
        ;
    }
}
#endif

#ifdef TASK2
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
    {1, 100, 405, 405, "Task 1", "[32;1m"},
    {2, 500, 1000, 1000, "Task 2", "[33;1m"}
};

const int iNumAsyncs = 1;
TaskInfo_t asyncs[] = 
{
    {1, 400, 1000, 1000, "ASYNC 1", "[34;1m"},
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
    TaskInfo_t* xTaskInfo = (TaskInfo_t*) pParam;    
    uint32_t first = 1;
    if (xTaskInfo->iTaskNumber == 1) {
        first = 0;
    }
    while(1) {

        printk("\033%s[ Task %d ] Started at %u\r\n\033[0m", xTaskInfo->color,
                                                             xTaskInfo->iTaskNumber,
                                                             xTaskGetTickCount());
        if (first && xTaskInfo->iTaskNumber == 2) {
            first = 0;
            vBusyWait(xTaskInfo->xWCET - 100);
        }
        else {
            vBusyWait(xTaskInfo->xWCET);
        }

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

        printk("\033%s[ Async Task %d ] at %u\r\n\033[0m", xTaskInfo->color,
                                                           xTaskInfo->iTaskNumber,
                                                           xTaskGetTickCount());
        vServerCBSNotify(server, (TaskFunction_t) AsyncJob, (void *) pcDebug);
        vTaskDelete( NULL );
}

void AsyncJob(void *pParam)
{
    //char *pcString = (char *) pParam;
    printk("\033[36;1m[ Async Job ] Started at %u\r\n\033[0m", xTaskGetTickCount());
    //printk("Async Job obtained: %s\r\n", pcString);
    //printk("Busy wait for fun!\r\n");
    uint32_t i;
    for (i=0; i<400*700; i++);
    printk("\033[36;1m[ Async Job ] Ended at %u\r\n\033[0m", xTaskGetTickCount());    
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
    for (int iTaskNum = 0; iTaskNum < iNumTasks; iTaskNum++)
    {
        xTaskCreate(TimingTestTask, tasks[iTaskNum].name, 256, (void *) &tasks[iTaskNum],
                    PRIORITY_EDF, tasks[iTaskNum].xWCET, tasks[iTaskNum].xRelativeDeadline,
                    tasks[iTaskNum].xPeriod, NULL);
    }
    
    // Create ASYNCS
    for (int iTaskNum = 0; iTaskNum < iNumAsyncs; iTaskNum++)
    {
        xTaskCreate(AsyncTestTask, asyncs[iTaskNum].name, 256, (void *) &asyncs[iTaskNum],
                    2, asyncs[iTaskNum].xWCET, asyncs[iTaskNum].xRelativeDeadline,
                    asyncs[iTaskNum].xPeriod, NULL);
    }

    // Create the server:
    xServerCBSCreate(CBSServer, "CBS Server", 256, NULL, PRIORITY_EDF, 300, 1200, &server);
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

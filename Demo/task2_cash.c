#ifdef TASK2_CASH
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
const int iNumTasks = 3;
TaskInfo_t tasks[] =
{
    {1, 1, 400, 400, "Task 1", "[32;1m"},
    {2, 1, 1000, 1000, "Task 2", "[33;1m"},
    {3, 1, 10000, 10000, "Task 2", "[33;1m"}
};

const int iNumAsyncs = 3;
TaskInfo_t asyncs[] = 
{
    {1, 100, 400, 400, "ASYNC 1", "[34;1m"},
    {2, 500, 1000, 1000, "ASYNC 2", "[34;1m"},
    {3, 700, 1200, 1200, "ASYNC 3", "[34;1m"}
};

typedef void (*vCB_t(int, void(*)(int)))(int);

// TODO Setup job fifo
vCB_t* pvAperiodicRequests[1];
int iNumAperiodicRequests = 0;

void AsyncJob1(void *pParam);
void AsyncJob2(void *pParam);
void AsyncJob3(void *pParam);

// Task never delays itself, always syspends itself
TaskHandle_t server1;
TaskHandle_t server2;
TaskHandle_t server3;

void delay(uint32_t ms) {
    int i;
    for(i=0; i<ms*610; i++);
}

void CBSServer(void* pParam)
{
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
            delay(xTaskInfo->xWCET);
        }
        else {
            delay(xTaskInfo->xWCET);
        }

        printk("\033%s[ Task %d ] Ended at %u\r\n\033[0m", xTaskInfo->color,
                                                           xTaskInfo->iTaskNumber,
                                                           xTaskGetTickCount());
        vEndTaskPeriod();
    }
}

const char* pcDebug = "Hello World!";

void AsyncTestTask1(void *pParam)
{
    TaskInfo_t* xTaskInfo = (TaskInfo_t*) pParam;

    printk("\033%s[ Async Task %d ] at %u\r\n\033[0m", xTaskInfo->color,
           xTaskInfo->iTaskNumber,
           xTaskGetTickCount());
    while (1) {
        vServerCBSNotify(server1, (TaskFunction_t) AsyncJob1, (void *) pcDebug);
        vEndTaskPeriod();
    }
}

void AsyncTestTask2(void *pParam)
{
    TaskInfo_t* xTaskInfo = (TaskInfo_t*) pParam;
    
    printk("\033%s[ Async Task %d ] at %u\r\n\033[0m", xTaskInfo->color,
           xTaskInfo->iTaskNumber,
           xTaskGetTickCount());
    while (1) {
        vServerCBSNotify(server2, (TaskFunction_t) AsyncJob2, (void *) pcDebug);
        vEndTaskPeriod();
    }
}

void AsyncTestTask3(void *pParam)
{
    TaskInfo_t* xTaskInfo = (TaskInfo_t*) pParam;
    
    printk("\033%s[ Async Task %d ] at %u\r\n\033[0m", xTaskInfo->color,
           xTaskInfo->iTaskNumber,
           xTaskGetTickCount());
    while (1) {
        vServerCBSNotify(server3, (TaskFunction_t) AsyncJob3, (void *) pcDebug);
        vEndTaskPeriod();
    }
}

void AsyncJob1(void *pParam)
{
    //char *pcString = (char *) pParam;
    portDISABLE_INTERRUPTS();
    printk("\033[36;1m[ Async Job 1 ] Started at %u\r\n\033[0m", xTaskGetTickCount());
    portENABLE_INTERRUPTS();
    //printk("Async Job obtained: %s\r\n", pcString);
    //printk("Busy wait for fun!\r\n");
    delay(100);
    portDISABLE_INTERRUPTS();    
    printk("\033[36;1m[ Async Job 1 ] Ended at %u\r\n\033[0m", xTaskGetTickCount());
    portENABLE_INTERRUPTS();    
}

uint32_t finishEarlier = 1;
void AsyncJob2(void *pParam)
{
    //char *pcString = (char *) pParam;
    portDISABLE_INTERRUPTS();    
    printk("\033[36;1m[ Async Job 2 ] Started at %u\r\n\033[0m", xTaskGetTickCount());
    portENABLE_INTERRUPTS();    
    //printk("Async Job obtained: %s\r\n", pcString);
    //printk("Busy wait for fun!\r\n");
    if (finishEarlier) {
        finishEarlier = 0;
        delay(400);
    }
    else {
        delay(500);
    }
    portDISABLE_INTERRUPTS();    
    printk("\033[36;1m[ Async Job 2 ] Ended at %u\r\n\033[0m", xTaskGetTickCount());
    portENABLE_INTERRUPTS();    
}

void AsyncJob3(void *pParam)
{
    //char *pcString = (char *) pParam;
    portDISABLE_INTERRUPTS();    
    printk("\033[36;1m[ Async Job 3 ] Started at %u\r\n\033[0m", xTaskGetTickCount());
    portENABLE_INTERRUPTS();    
    //printk("Async Job obtained: %s\r\n", pcString);
    //printk("Busy wait for fun!\r\n");
    delay(400);
    portDISABLE_INTERRUPTS();    
    printk("\033[36;1m[ Async Job 3 ] Ended at %u\r\n\033[0m", xTaskGetTickCount());
    portENABLE_INTERRUPTS();    
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

    TaskFunction_t asyncFuncs[] = {AsyncTestTask1, AsyncTestTask2, AsyncTestTask3};
    uint32_t i;
    for (i=0; i<iNumAsyncs; i++) {
        // Create ASYNCS
        xTaskCreate(asyncFuncs[i], asyncs[i].name, 256, (void *) &asyncs[i],
                    2, asyncs[i].xWCET, asyncs[i].xRelativeDeadline,
                    asyncs[i].xPeriod, NULL);
    }

    // Create the server:
    xServerCBSCreate(CBSServer, "CBS Server 1", 256, NULL, PRIORITY_EDF, 100, 400, &server1);
    xServerCBSCreate(CBSServer, "CBS Server 2", 256, NULL, PRIORITY_EDF, 500, 1000, &server2);
    xServerCBSCreate(CBSServer, "CBS Server 3", 256, NULL, PRIORITY_EDF, 300, 1180, &server3);
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

#ifdef TASK3
#include <FreeRTOS.h>
#include <task.h>
#include <printk.h>

#include "Drivers/rpi_gpio.h"
#include "Drivers/rpi_irq.h"
#include "Drivers/rpi_aux.h"

volatile static ResourceHandle_t xR1;
volatile static uint32_t ulCounter;


typedef struct TaskInfo_s
{
    int iTaskNumber;
    TickType_t xWCET;
    TickType_t xPeriod;
    TickType_t xRelativeDeadline;
    const char* name;
    const char* color;
    uint32_t ulCount;
    volatile ResourceHandle_t* pxRes;
} TaskInfo_t;

// Define tasks
const int iNumTasks = 3;
TaskInfo_t tasks[] =
{
    {1, 50, 200, 200, "Task 1", "[32;1m", 0, NULL},
    {2, 100, 300, 300, "Task 2", "[33;1m", 1000, &xR1},
    {3, 150, 400, 400, "Task 3", "[34;1m", 3000, &xR1},
};

void TimingTestTask(void *pParam) {
    while(1) {
        TaskInfo_t* xTaskInfo = (TaskInfo_t*) pParam;

        printk("\033%s[ Task %d ] Started at %u\r\n\033[0m", xTaskInfo->color,
                                                             xTaskInfo->iTaskNumber,
                                                             xTaskGetTickCount());

        if (xTaskInfo->pxRes != NULL)
        {

            BaseType_t vVal = vSRPSemaphoreTake(*(xTaskInfo->pxRes), portMAX_DELAY);
            configASSERT(vVal == pdTRUE);
            printk("\033%s[ Task %d ] Obtained Resource\r\n\033[0m", xTaskInfo->color,
                                                                     xTaskInfo->iTaskNumber);
            printk("\033%s[ Task %d ] Start Counter: %u\r\n\033[0m", xTaskInfo->color,
                                                                     xTaskInfo->iTaskNumber,
                                                                     ulCounter);

            for(uint32_t ulI = 0; ulI < xTaskInfo->ulCount; ulI++)
                ulCounter++;

            printk("\033%s[ Task %d ] End Counter: %u\r\n\033[0m", xTaskInfo->color,
                                                                   xTaskInfo->iTaskNumber,
                                                                   ulCounter);
            vVal = vSRPSemaphoreGive(*(xTaskInfo->pxRes));
            configASSERT(vVal == pdTRUE);
            printk("\033%s[ Task %d ] Released Resource\r\n\033[0m", xTaskInfo->color,
                                                                     xTaskInfo->iTaskNumber);
        }


        vBusyWait(xTaskInfo->xWCET);

        printk("\033%s[ Task %d ] Ended at %u\r\n\033[0m", xTaskInfo->color,
                                                           xTaskInfo->iTaskNumber,
                                                           xTaskGetTickCount());
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

    BaseType_t vVal;

    vVal = vSRPInitSRP();
    if (vVal == pdFALSE) {
        printk("Failed to initialize SRP stacks\r\n");
        configASSERT(pdFALSE);
    }
    else {
        printk("Successfully initialized SRP stacks\r\n");
    }


    ulCounter = 0;

    TaskHandle_t pxTaskHandles[iNumTasks];
    // Create tasks
    for (int iTaskNum = 0; iTaskNum < iNumTasks; iTaskNum++)
    {
        xTaskCreate(TimingTestTask, tasks[iTaskNum].name, 256, (void *) &tasks[iTaskNum],
                    PRIORITY_EDF, tasks[iTaskNum].xWCET, tasks[iTaskNum].xRelativeDeadline,
                    tasks[iTaskNum].xPeriod, &pxTaskHandles[iTaskNum]);
    }

    xR1 = vSRPSemaphoreCreateBinary();
    if (xR1 == NULL) {
        printk("Failed to create resource\r\n");
    }
    else {
        printk("Resource created successfully\r\n");
    }

    srpConfigToUseResource(xR1, pxTaskHandles[1]);
    srpConfigToUseResource(xR1, pxTaskHandles[2]);

    vPrintSchedule();
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


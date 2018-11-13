#ifdef TASK2_OVER
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

//Overrun Demo
TaskInfo_t overrunDemo[] =
{
    {1, 1000, 2000, 2000, "Task 1", "[32;1m"},
    {2, 1000, 4000, 4000, "Task 2", "[33;1m"},
};

const int iNumTasks = 2;
TaskInfo_t* tasks = overrunDemo;

BaseType_t over = pdFALSE;

void TimingTestTask(void *pParam) {
    while(1) {
        TaskInfo_t* xTaskInfo = (TaskInfo_t*) pParam;

        printk("\033%s[ Task %d ] Started at %u\r\n\033[0m", xTaskInfo->color,
                                                             xTaskInfo->iTaskNumber,
                                                             xTaskGetTickCount());

        vBusyWait(xTaskInfo->xWCET);

        if (over == pdFALSE)
        {
            over = pdTRUE;
            vBusyWait(100*xTaskInfo->xWCET);
            printk("THIS SHOULD NEVER PRINT%d\r\n", xTaskInfo->iTaskNumber);
        }

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

    // Create tasksk
    for (int iTaskNum = 0; iTaskNum < iNumTasks; iTaskNum++)
    {
        xTaskCreate(TimingTestTask, tasks[iTaskNum].name, 256, (void *) &tasks[iTaskNum],
                    PRIORITY_EDF, tasks[iTaskNum].xWCET, tasks[iTaskNum].xRelativeDeadline,
                    tasks[iTaskNum].xPeriod, NULL);
    }
    printk("Finish creating tasks\r\n");

    vPrintSchedule();
    vVerifyEDFExactBound();
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


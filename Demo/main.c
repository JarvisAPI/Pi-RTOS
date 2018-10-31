#include <FreeRTOS.h>
#include <task.h>
#include <printk.h>

#include "Drivers/rpi_gpio.h"
#include "Drivers/rpi_irq.h"
#include "Drivers/rpi_aux.h"

// Number of increments it takes to wait for 1ms, with tick interrupts happening in between
// This value was obtained by using binary search on a single task with 10ms deadline
#define MAGIC_COUNT 745

static TickType_t xStartTime = 0;


typedef struct TaskInfo_s
{
    int iTaskNumber;
    TickType_t xWCET;
    TickType_t xPeriod;
    TickType_t xRelativeDeadline;
} TaskInfo_t;


// Define tasks
const int iNumTasks = 3;
TaskInfo_t tasks[] =
{
    {1, 800, 3000, 2000},
    {2, 1800, 7000, 5500},
    {3, 1800, 10000, 6000}
};


void TimingTestTask(void *pParam) {
    TickType_t xLastWakeTime = xStartTime;
    TaskInfo_t* xTaskInfo = (TaskInfo_t*) pParam;
    while(1) {
        printk("Start Timing task %d\r\n", xTaskInfo->iTaskNumber);
        busyWait(xTaskInfo->xWCET);
        printk("Done Timing task %d\r\n", xTaskInfo->iTaskNumber);
        vTaskDelayUntil( &xLastWakeTime, xTaskInfo->xPeriod );
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

    rpi_gpio_sel_fun(47, 1);			// RDY led
    rpi_gpio_sel_fun(35, 1);			// RDY led


    // Create tasksk
    for (int iTaskNum = 0; iTaskNum < iNumTasks; iTaskNum++)
    {
        xTaskCreate(TimingTestTask, "Timing Task", 256, (void *) &tasks[iTaskNum], PRIORITY_EDF,
                    tasks[iTaskNum].xWCET, tasks[iTaskNum].xRelativeDeadline,
                    tasks[iTaskNum].xPeriod, NULL);
    }

    //xTaskCreate(TimingTestTask2, "LED_1", 256, NULL, 1, 1000, 2500, xTimingDelay2, NULL);
    //xTaskCreate(TimingTestTask3, "LED_2", 256, NULL, 1, 1500, 3500, xTimingDelay3, NULL);

    printSchedule();
    //verifyEDFExactBound();
    xStartTime = xTaskGetTickCount();
    vTaskStartScheduler();

    /*
     *	We should never get here, but just in case something goes wrong,
     *	we'll place the CPU into a safe loop.
     */
    while(1) {
        ;
    }
}


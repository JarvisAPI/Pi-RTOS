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
    const char* name;
} TaskInfo_t;


// Define tasks
TaskInfo_t edfLStarDemo[] =
{
    {1, 900, 3000, 2000, "Task 1"},
    {2, 1900, 7000, 5500, "Task 2"},
    {3, 1900, 10000, 6000, "Task 3"}
};

//Overrun Demo
TaskInfo_t overrunDemo[] =
{
    {1, 1000, 2000, 2000, "Task 1"},
    {2, 1000, 4000, 4000, "Task 2"},
};

const int iNumTasks = 2;
TaskInfo_t* tasks = overrunDemo;

BaseType_t wow = pdFALSE;

void TimingTestTask(void *pParam) {
    TaskInfo_t* xTaskInfo = (TaskInfo_t*) pParam;
    while(1) {
        printk("Started Task iter at: %u\r\n", xTaskGetTickCount());
        printk("Start Timing task %d\r\n", xTaskInfo->iTaskNumber);
        busyWait(xTaskInfo->xWCET);
        if (wow == pdFALSE)
        {
            wow = pdTRUE;
            busyWait(xTaskInfo->xWCET+10000);
            printk("THIS SHOULD NEVER PRINT%d\r\n", xTaskInfo->iTaskNumber);
        }
        printk("Done Timing task %d\r\n", xTaskInfo->iTaskNumber);
        endTaskPeriod();
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
        xTaskCreate(TimingTestTask, tasks[iTaskNum].name, 256, (void *) &tasks[iTaskNum],
                    PRIORITY_EDF, tasks[iTaskNum].xWCET, tasks[iTaskNum].xRelativeDeadline,
                    tasks[iTaskNum].xPeriod, NULL);
    }

    printSchedule();
    verifyEDFExactBound();
    verifyLLBound();
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


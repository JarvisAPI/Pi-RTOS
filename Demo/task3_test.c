#include <FreeRTOS.h>
#include <task.h>
#include <printk.h>

#include "Drivers/rpi_gpio.h"
#include "Drivers/rpi_irq.h"
#include "Drivers/rpi_aux.h"
#include "info_task3_test.h"

extern TaskInfo_t tasks[];
static int iNumTasks = 3;

void TimingTestTask(void *pParam) {
    while(1) {
        TaskInfo_t* xTaskInfo = (TaskInfo_t*) pParam;        
        printk("Start %s\r\n", xTaskInfo->name);
        busyWait(xTaskInfo->xWCET - 3);
        printk("Done %s\r\n", xTaskInfo->name);
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

    BaseType_t vVal;

    vVal = srpInitSRPStacks();
    if (vVal == pdFALSE) {
        printk("Failed to initialize SRP stacks\r\n");
        while (1) {
            
        }
    }
    else {
        printk("Successfully initialized SRP stacks\r\n");
    }    
    
    // Create tasksk
    for (int iTaskNum = 0; iTaskNum < iNumTasks; iTaskNum++)
    {
        xTaskCreate(TimingTestTask, tasks[iTaskNum].name, 0, (void *) &tasks[iTaskNum],
                    PRIORITY_EDF, tasks[iTaskNum].xWCET, tasks[iTaskNum].xRelativeDeadline,
                    tasks[iTaskNum].xPeriod, NULL);
    }

    printSchedule();
    verifyEDFExactBound();
    //verifyLLBound();
    vTaskStartScheduler();

    /*
     *	We should never get here, but just in case something goes wrong,
     *	we'll place the CPU into a safe loop.
     */
    while(1) {
        ;
    }
}


#ifdef TASK3_RTS2
#include <FreeRTOS.h>
#include <task.h>
#include <printk.h>

#include "Drivers/rpi_gpio.h"
#include "Drivers/rpi_irq.h"
#include "Drivers/rpi_aux.h"

volatile static ResourceHandle_t xR1;
volatile static uint32_t ulC1 = 0;

volatile static ResourceHandle_t xR2;
volatile static uint32_t ulC2 = 0;

volatile static ResourceHandle_t xR3;
volatile static uint32_t ulC3 = 0;

typedef struct TaskInfo_s
{
    uint32_t xTaskNum;
    TickType_t xWCET;
    TickType_t xPeriod;
    TickType_t xRelativeDeadline;
    const char *name;
    const char *color;
    uint32_t xCompletedPhaseShift;
} TaskInfo_t;

const int iNumTasks = 4;
TaskInfo_t tasks[] =
{
    {1, 160, 550, 550, "Task 1", "[35;1m", 0},
    {2, 160, 650, 650, "Task 2", "[34;1m", 0},
    {3, 110, 800, 800, "Task 3", "[33;1m", 0},    
    {4, 210, 900, 900, "Task 4", "[32;1m", 1}
};

void vExecTask1(void) {
    vSRPSemaphoreTake(xR1, portMAX_DELAY);
    configASSERT(ulC1 == 0);
    ulC1 = 1;
    vBusyWait2(48);
    configASSERT(ulC1 == 1);
    ulC1 = 0;
    vSRPSemaphoreGive(xR1);

    vSRPSemaphoreTake(xR2, portMAX_DELAY);
    configASSERT(ulC2 == 0);
    ulC2 = 1;
    vBusyWait2(48);
    configASSERT(ulC2 == 1);
    ulC2 = 0;
    vSRPSemaphoreGive(xR2);

    vSRPSemaphoreTake(xR3, portMAX_DELAY);
    configASSERT(ulC3 == 0);
    ulC3 = 1;
    vBusyWait2(48);
    configASSERT(ulC3 == 1);
    ulC3 = 0;
    vSRPSemaphoreGive(xR3);
}

void vExecTask2(void) {
    vSRPSemaphoreTake(xR2, portMAX_DELAY);
    configASSERT(ulC2 == 0);
    ulC2 = 1;
    vBusyWait2(98);
    configASSERT(ulC2 == 1);
    ulC2 = 0;
    vSRPSemaphoreGive(xR2);

    vBusyWait2(48);
}

void vExecTask3(void) {
    vBusyWait2(55);

    vSRPSemaphoreTake(xR3, portMAX_DELAY);
    configASSERT(ulC3 == 0);
    ulC3 = 1;
    vBusyWait2(48);
    configASSERT(ulC3 == 1);
    ulC3 = 0;
    vSRPSemaphoreGive(xR3);
}

void vExecTask4(void) {
    vSRPSemaphoreTake(xR1, portMAX_DELAY);
    configASSERT(ulC1 == 0);
    ulC1 = 1;
    vBusyWait2(98);
    configASSERT(ulC1 == 1);
    ulC1 = 0;
    vSRPSemaphoreGive(xR1);

    vBusyWait2(48);
}

void TimingTestTask(void *pParam) {
    TaskInfo_t* xTaskInfo = (TaskInfo_t*) pParam;            
    if (xTaskInfo->xCompletedPhaseShift == 0) {
        xTaskInfo->xCompletedPhaseShift = 1;
        switch(xTaskInfo->xTaskNum) {
        case 1: vEndTask(200); break;
        case 2: vEndTask(150); break;
        case 3: vEndTask(50); break;
        }
    }
    
    while(1) {
        printk("\033%s[ %s ] Started at %u\033[0m\r\n",
               xTaskInfo->color,
               xTaskInfo->name,
               xTaskGetTickCount());
        switch(xTaskInfo->xTaskNum) {
        case 1: vExecTask1(); break;
        case 2: vExecTask2(); break;
        case 3: vExecTask3(); break;
        case 4: vExecTask4(); break;
        }        
        printk("\033%s[ %s ] Ended at %u\033[0m\r\n",
               xTaskInfo->color,
               xTaskInfo->name,
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
    
    TaskHandle_t pxTaskHandles[iNumTasks];
    BaseType_t vVal;

    vVal = vSRPInitSRP();
    if (vVal == pdFALSE) {
        printk("Failed to initialize SRP stacks\r\n");
        while (1) {
            
        }
    }
    else {
        printk("Successfully initialized SRP stacks\r\n");
    }    
    
    // Create tasks
    for (int iTaskNum = 0; iTaskNum < iNumTasks; iTaskNum++)
    {
        xTaskCreate(TimingTestTask, tasks[iTaskNum].name, 256, (void *) &tasks[iTaskNum],
                    PRIORITY_EDF, tasks[iTaskNum].xWCET, tasks[iTaskNum].xRelativeDeadline,
                    tasks[iTaskNum].xPeriod, &pxTaskHandles[iTaskNum]);
    }

    xR1 = vSRPSemaphoreCreateBinary();
    xR2 = vSRPSemaphoreCreateBinary();
    xR3 = vSRPSemaphoreCreateBinary();

    if (xR1 == NULL || xR2 == NULL || xR3 == NULL) {
        printk("Failed to create resources\r\n");
        while (1) {
        }
    }
    else {
        printk("Resources created successfully\r\n");
    }

    srpConfigToUseResource(xR1, pxTaskHandles[0]);
    srpConfigToUseResource(xR2, pxTaskHandles[0]);
    srpConfigToUseResource(xR3, pxTaskHandles[0]);

    srpConfigToUseResource(xR2, pxTaskHandles[1]);

    srpConfigToUseResource(xR3, pxTaskHandles[2]);    
    
    srpConfigToUseResource(xR1, pxTaskHandles[3]);    

    vPrintSchedule();
    vVerifyEDFExactBound();
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
#endif

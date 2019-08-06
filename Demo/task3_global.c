#ifdef TASK3_GLOBAL
#include <FreeRTOS.h>
#include <task.h>
#include <printk.h>
#include <portmacro.h>

#include "Drivers/rpi_gpio.h"
#include "Drivers/rpi_irq.h"
#include "Drivers/rpi_aux.h"
#include "Drivers/rpi_core.h"
#include "Drivers/rpi_mmu.h"
#include "Drivers/rpi_timer.h"
#include "Drivers/rpi_memory.h"

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
const int iNumTasks = 4;
TaskInfo_t tasks[] =
{
    {1, 600, 2000, 2000, "Task 1", "[32;1m"},
    {2, 1500, 5000, 5000, "Task 2", "[33;1m"},
    {3, 1000, 4000, 4000, "Task 3", "[34;1m"},
    {4, 1000, 4000, 4000, "Task 4", "[35;1m"}
};

extern uint32_t CPUID(void);

void TimingTestTask(void *pParam) {
    TaskInfo_t* xTaskInfo = (TaskInfo_t*) pParam;

    while(1) {
        portDISABLE_INTERRUPTS();

        printk("\033%s[ Task %d ] Started at %u\r\n\033[0m", xTaskInfo->color,
                                                             xTaskInfo->iTaskNumber,
                                                             xTaskGetTickCount());
        portENABLE_INTERRUPTS();

        int val = xTaskInfo->xWCET;
        int i;
        for(i=0;i<val*70000; i++);

        portDISABLE_INTERRUPTS();
        printk("\033%s[ Task %d ] Ended at %u\r\n\033[0m", xTaskInfo->color,
                                                           xTaskInfo->iTaskNumber,
                                                           xTaskGetTickCount());
        portENABLE_INTERRUPTS();
        vEndTaskPeriod();
    }
}


void und_c_handler(void) {
    printk("Undefined Instruction, cpuid: %u!\r\n", CPUID());
}

void create_tasks(void) {
    int i;
    for (i=0; i<iNumTasks; i++) {
        xTaskCreate(TimingTestTask, tasks[i].name, 256, (void *) &tasks[i],
                    PRIORITY_EDF, tasks[i].xWCET, tasks[i].xRelativeDeadline,
                    tasks[i].xPeriod, NULL);
    }

}


extern void prvTaskExitError(void);
int main(void)
{
    rpi_aux_mu_init();

    printk("Primary Core Started\r\n");

    mmu_init_page_table();
    mmu_init();

    uint32_t numCoresUsed = 4;
    vInitCoreStructures(numCoresUsed);
    smp_init();

    rpi_irq_init();
    rpi_cpu_irq_enable();

    printk("Creating tasks\r\n");
    create_tasks();
    printk("Starting Scheduler\r\n");
    printk("RetAddr: 0x%x\r\n", prvTaskExitError);

    vTaskStartScheduler();

    while (1) {
    }
}

#endif

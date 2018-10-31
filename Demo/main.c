#include <FreeRTOS.h>
#include <task.h>
#include <printk.h>

#include "Drivers/rpi_gpio.h"
#include "Drivers/rpi_irq.h"
#include "Drivers/rpi_aux.h"

// Number of increments it takes to wait for 1ms, with tick interrupts happening in between
// This value was obtained by using binary search on a single task with 10ms deadline
#define MAGIC_COUNT 745

const TickType_t xDelay = 10 / portTICK_PERIOD_MS;

const char *str0 = "Ok I am Task1";
const char *str1 = "Hey Task2 here";

const TickType_t xTimingDelay1 = 2000 / portTICK_PERIOD_MS;
const TickType_t xTimingDelay2 = 3000 / portTICK_PERIOD_MS;
const TickType_t xTimingDelay3 = 4000 / portTICK_PERIOD_MS;


volatile static TickType_t xStartTime = 0;
void TimingTestTask1(void *pParam) {
    TickType_t xLastWakeTime;
    xLastWakeTime = xTaskGetTickCount();
    while(1) {
        printk("Start TimingTask1\r\n");
        busyWait( 200 );
        printk("End TimingTask1\r\n");
        vTaskDelayUntil( &xLastWakeTime, xTimingDelay1 );
    }
}

void TimingTestTask2(void *pParam) {
    TickType_t xLastWakeTime;
    xLastWakeTime = xStartTime;
    while(1) {
        printk("Start TimingTask2\r\n");
        busyWait( 1000 );
        printk("End TimingTask2\r\n");
        vTaskDelayUntil( &xLastWakeTime, xTimingDelay2 );
    }
}

void TimingTestTask3(void *pParam) {
    TickType_t xLastWakeTime;
    xLastWakeTime = xStartTime;
    while(1) {
        printk("Start TimingTask3: %u\r\n", getTime());
        busyWait( 1500 );
        printk("End TimingTask3: %u\r\n", getTime());
        vTaskDelayUntil( &xLastWakeTime, xTimingDelay3 );
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

    //xTaskCreate(task1, "LED_0", 128, (void *) 8, 0, 0,NULL);
    //xTaskCreate(task2, "LED_1", 128, (void *) 4, 0, 0,NULL);

    xTaskCreate(TimingTestTask1, "LED_0", 256, NULL, 1, 200, xTimingDelay1, xTimingDelay1, NULL);
    xTaskCreate(TimingTestTask2, "LED_1", 256, NULL, 1, 1000, xTimingDelay2, xTimingDelay2, NULL);
    xTaskCreate(TimingTestTask3, "LED_2", 256, NULL, 1, 1500, xTimingDelay3, xTimingDelay3, NULL);

    printSchedule();
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


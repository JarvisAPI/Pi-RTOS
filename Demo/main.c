#include <FreeRTOS.h>
#include <task.h>
#include <printk.h>

#include "Drivers/rpi_gpio.h"
#include "Drivers/rpi_irq.h"
#include "Drivers/rpi_aux.h"

const TickType_t xDelay = 500 / portTICK_PERIOD_MS;

const char *str0 = "Ok I am Task1";
const char *str1 = "Hey Task2 here";

const TickType_t xTimingDelay1 = 1000 / portTICK_PERIOD_MS;
const TickType_t xTimingDelay2 = 3000 / portTICK_PERIOD_MS;

void task1(void *pParam) {
	int i = 0;
	while(1) {
		i++;
        rpi_aux_mu_string((char *)str0);
        printk("TimingTask1\r\n");
		rpi_gpio_set_val(47, 1);
		rpi_gpio_set_val(35, 0);
		vTaskDelay(xDelay);
	}
}

void task2(void *pParam) {
	int i = 0;
	while(1) {
		i++;
        rpi_aux_mu_string((char *)str1);
		rpi_gpio_set_val(47, 0);
		rpi_gpio_set_val(35, 1);
		vTaskDelay(xDelay/2);
	}
}

void TimingTestTask1(void *pParam) {
    TickType_t xLastWakeTime;
    xLastWakeTime = xTaskGetTickCount();
    while(1) {
        printk("TimingTask1\r\n");
        vTaskDelayUntil( &xLastWakeTime, xTimingDelay1 );
    }
}

void TimingTestTask2(void *pParam) {
    TickType_t xLastWakeTime;
    xLastWakeTime = xTaskGetTickCount();
    while(1) {
        for( int i = 0; i < 2000000; i++);
        printk("TimingTask2\r\n");
        vTaskDelayUntil( &xLastWakeTime, xTimingDelay2 );
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

    xTaskCreate(TimingTestTask2, "LED_1", 128, (void *) 4, 1, xTimingDelay2, NULL);
    xTaskCreate(TimingTestTask1, "LED_0", 128, (void *) 8, 1, xTimingDelay1, NULL);

    printSchedule();
    vTaskStartScheduler();

    /*
     *	We should never get here, but just in case something goes wrong,
     *	we'll place the CPU into a safe loop.
     */
    while(1) {
        ;
    }
}


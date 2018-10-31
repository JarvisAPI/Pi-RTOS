#include <FreeRTOS.h>
#include <task.h>
#include <printk.h>

#include "Drivers/rpi_gpio.h"
#include "Drivers/rpi_irq.h"
#include "Drivers/rpi_aux.h"

const TickType_t xTimingDelay1 = 200 / portTICK_PERIOD_MS;
const TickType_t xTimingDelay2 = 300 / portTICK_PERIOD_MS;
//const TickType_t xTimingDelay3 = 4000 / portTICK_PERIOD_MS;

volatile static ResourceHandle_t mR1;
volatile static uint32_t mCounter;

volatile static TickType_t xStartTime = 0;

void TimingTestTask1(void *pParam) {
    TickType_t xLastWakeTime;
    BaseType_t vVal;
    int i;
    
    xLastWakeTime = xTaskGetTickCount();
    while(1) {
        printk("[Task1]: Running\r\n");
        printk("[Task1]: Acquiring resource R1\r\n");
        vVal = srpSemaphoreTake(mR1, portMAX_DELAY);
        if ( vVal == pdTRUE ) {
            printk("[Task1]: Obtained resource R1!\r\n");
            printk("[Task1]: Starting counter: %u\r\n", mCounter);
            for (i=0; i<1000; i++) {
                mCounter++;
            }
            printk("[Task1]: Ending counter: %u\r\n", mCounter);
            printk("[Task1]: Releasing resource R1\r\n");
            vVal = srpSemaphoreGive(mR1);
            printk("[Task1]: Released resource R1!\r\n");            
        }
        else {
            printk("[Task1]: Failed to obtain resource\r\n");
        }
        vTaskDelayUntil( &xLastWakeTime, xTimingDelay1 );
    }
}

void TimingTestTask2(void *pParam) {
    TickType_t xLastWakeTime;
    BaseType_t vVal;
    int i;
    xLastWakeTime = xStartTime;
    
    while(1) {
        printk("[Task2]: Running\r\n");
        printk("[Task2]: Acquiring resource R1\r\n");
        vVal = srpSemaphoreTake(mR1, portMAX_DELAY);
        if ( vVal == pdTRUE ) {
            printk("[Task2]: Obtained resource R1!\r\n");
            printk("[Task2]: Starting counter: %u\r\n", mCounter);
            for (i=0; i<2000; i++) {
                mCounter++;
            }
            printk("[Task2]: Ending counter: %u\r\n", mCounter);
            printk("[Task2]: Releasing resource R1\r\n");
            vVal = srpSemaphoreGive(mR1);
            printk("[Task2]: Released resource R1!\r\n");            
        }
        else {
            printk("[Task2]: Failed to obtain resource\r\n");
        }
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

    TaskHandle_t mTask1, mTask2;
    BaseType_t vVal;
    mCounter = 0;
    
    xTaskCreate(TimingTestTask1, "LED_0", 256, NULL, 1, 50, xTimingDelay1, &mTask1);
    xTaskCreate(TimingTestTask2, "LED_1", 256, NULL, 1, 50, xTimingDelay2, &mTask2);

    vVal = srpInitSRPStacks();
    if (vVal == pdFALSE) {
        printk("Failed to initialize SRP stacks\r\n");
        while (1) {
            
        }
    }
    else {
        printk("Successfully initialized SRP stacks\r\n");
    }

    mR1 = srpSemaphoreCreateBinary();
    if (mR1 == NULL) {
        printk("Failed to create resource\r\n");
    }
    else {
        printk("Resource created successfully\r\n");
    }
    printk("After creation Resouce: 0x%x\r\n", mR1);
    
    srpConfigToUseResource(mR1, mTask1);
    srpConfigToUseResource(mR1, mTask2);

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


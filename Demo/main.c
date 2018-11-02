#include <FreeRTOS.h>
#include <task.h>
#include <printk.h>

#include "Drivers/rpi_gpio.h"
#include "Drivers/rpi_irq.h"
#include "Drivers/rpi_aux.h"

const TickType_t xTimingDelay1 = 200 / portTICK_PERIOD_MS;
const TickType_t xTimingDelay2 = 300 / portTICK_PERIOD_MS;
const TickType_t xTimingDelay3 = 400 / portTICK_PERIOD_MS;

volatile static ResourceHandle_t mR1;
volatile static uint32_t mCounter;

extern uint32_t debugGetStack(void);

void TimingTestTask1(void *pParam) {
    while(1) {
        //int i = 0;
        printk("[Task1]: Running\r\n");
        printk("[Task1]: Stack: 0x%x\r\n", debugGetStack());
        //printk("[Task1]: Before i: %u\r\n", i);
        //i += 12;
        busyWait( 25 );
        //printk("[Task1]: After i: %u\r\n", i);
        printk("[Task1]: Finished\r\n");
        srpTaskDelay( xTimingDelay1 );
    }
}

void TimingTestTask2(void *pParam) {    
    while(1) {
        BaseType_t vVal;
        int i;
        
        printk("[Task2]: Running\r\n");
        printk("[Task2]: Acquiring resource R1\r\n");
        vVal = srpSemaphoreTake(mR1, portMAX_DELAY);
        if ( vVal == pdTRUE ) {
            printk("[Task2]: Obtained resource R1!\r\n");
            printk("[Task2]: Starting counter: %u\r\n", mCounter);
            for (i=0; i<1000; i++) {
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
        busyWait( 100 );
        printk("[Task2]: Finished\r\n");
        srpTaskDelay( xTimingDelay2 );
    }
}

void TimingTestTask3(void *pParam) {
    while(1) {
        BaseType_t vVal;
        int i;
        
        printk("[Task3]: Running\r\n");
        printk("[Task3]: Acquiring resource R1\r\n");
        vVal = srpSemaphoreTake(mR1, portMAX_DELAY);
        if ( vVal == pdTRUE ) {
            printk("[Task3]: Obtained resource R1!\r\n");
            printk("[Task3]: Starting counter: %u\r\n", mCounter);
            for (i=0; i<2000; i++) {
                mCounter++;
            }
            printk("[Task3]: Ending counter: %u\r\n", mCounter);
            printk("[Task3]: Releasing resource R1\r\n");
            vVal = srpSemaphoreGive(mR1);
            printk("[Task3]: Released resource R1!\r\n");            
        }
        else {
            printk("[Task3]: Failed to obtain resource\r\n");
        }
        busyWait( 150 );
        printk("[Task3]: Finished\r\n");
        srpTaskDelay( xTimingDelay3 );
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

    //rpi_gpio_sel_fun(47, 1);			// RDY led
    //rpi_gpio_sel_fun(35, 1);			// RDY led

    //TaskHandle_t mTask2, mTask3;
    BaseType_t vVal;
    //mCounter = 0;

    vVal = srpInitSRPStacks();
    
    if (vVal == pdFALSE) {
        printk("Failed to initialize SRP stacks\r\n");
        while (1) {
            
        }
    }
    else {
        printk("Successfully initialized SRP stacks\r\n");
    }

    xTaskCreate(TimingTestTask1, "TASK_0", 256, NULL, 1, 30, 200, xTimingDelay1, NULL);
    //xTaskCreate(TimingTestTask2, "TASK_1", 256, NULL, 1, 105, 300, xTimingDelay2, &mTask2);
    //xTaskCreate(TimingTestTask3, "TASK_2", 256, NULL, 1, 155, 400, xTimingDelay3, &mTask3);    

    /*
    mR1 = srpSemaphoreCreateBinary();
    if (mR1 == NULL) {
        printk("Failed to create resource\r\n");
    }
    else {
        printk("Resource created successfully\r\n");
    }*/
    
    //srpConfigToUseResource(mR1, mTask2);
    //srpConfigToUseResource(mR1, mTask3);

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


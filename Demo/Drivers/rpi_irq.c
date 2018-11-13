#include <stdint.h>

#include "rpi_irq.h"

static RPI_IRQ_TABLE_t g_rpi_irq_table[RPI_TOTAL_IRQ];

#define clz(a) \
 ({ unsigned long __value, __arg = (a); \
     asm ("clz\t%0, %1": "=r" (__value): "r" (__arg)); \
     __value; })

/**
 *	This is the global IRQ handler on this platform!
 *	It is based on the assembler code found in the Broadcom datasheet.
 *
 **/
void vApplicationIRQHandler() {
    register uint32_t ulMaskedStatus;
    register uint32_t irqNumber;
    register uint32_t tmp;

    ulMaskedStatus = RPI_IRQ->BASIC_PENDING;
    tmp = ulMaskedStatus & 0x00000300;	// Check if anything pending in pr1/pr2.

    if (ulMaskedStatus & ~0xFFFFF300) {	// Note how we mask out the GPU interrupt Aliases.
        irqNumber = 64 + 31;	// Shifting the basic ARM IRQs to be IRQ# 64 +
        goto emit_interrupt;
    }

    if (tmp & 0x100) {
        ulMaskedStatus = RPI_IRQ->PENDING_1;
        irqNumber = 0 + 31;
        // Clear the interrupts also available in basic IRQ pending reg.
        //ulMaskedStatus &= ~((1 << 7) | (1 << 9) | (1 << 10) | (1 << 18) | (1 << 19));
        if (ulMaskedStatus) {
            goto emit_interrupt;
        }
    }

    if (tmp & 0x200) {
        ulMaskedStatus = RPI_IRQ->PENDING_2;
        irqNumber = 32 + 31;
        // Don't clear the interrupts in the basic pending, simply allow them to processed here!
        if (ulMaskedStatus) {
            goto emit_interrupt;
        }
    }

    //return;

    emit_interrupt:

    tmp = ulMaskedStatus - 1;
    ulMaskedStatus = ulMaskedStatus ^ tmp;

    uint32_t lz = clz(ulMaskedStatus);
    irqNumber = irqNumber - lz;

    if (g_rpi_irq_table[irqNumber].pHandler) {
        g_rpi_irq_table[irqNumber].pHandler(irqNumber, g_rpi_irq_table[irqNumber].pParam);
    }
}

__attribute__((no_instrument_function))
static void stubHandler(int nIRQ, void *pParam) {
	/**
	 *	Actually if we get here, we should probably disable the IRQ,
	 *	otherwise we could lock up this system, as there is nothing to
	 *	ackknowledge the interrupt.
	 **/
}

void rpi_irq_init() {
    int i;

    RPI_IRQ->DISABLE_1 = RPI_IRQ_MASK;
    RPI_IRQ->DISABLE_2 = RPI_IRQ_MASK;
    RPI_IRQ->DISABLE_BASIC = RPI_IRQ_MASK;

    for (i = 0; i < RPI_TOTAL_IRQ; i++) {
        g_rpi_irq_table[i].pHandler = stubHandler;
        g_rpi_irq_table[i].pParam = (void *) 0;
    }
}

int rpi_irq_register_handler(uint32_t nIRQ, RPI_IRQ_HANDLER_t pHandler,
        void *pParam) {

    if (nIRQ >= RPI_TOTAL_IRQ) {
        return -1;
    }

    // Do we need stop cpu irq or just mask irq?
    //rpi_irq_disable(nIRQ);
    {
        g_rpi_irq_table[nIRQ].pHandler = pHandler;
        g_rpi_irq_table[nIRQ].pParam = pParam;
    }
    //rpi_irq_enable(nIRQ);

    return nIRQ;
}

int rpi_irq_enable(uint32_t nIRQ) {

    uint32_t bank = RPI_IRQ_ID_BANK(nIRQ);
    uint32_t mask = RPI_IRQ_ID_MASK(nIRQ);

    if (bank == 0) {
        RPI_IRQ->ENABLE_1 = mask;

    } else if (bank == 1) {
        RPI_IRQ->ENABLE_2 = mask;

    } else if (bank == 2) {
        RPI_IRQ->ENABLE_BASIC = mask;

    } else {
        return -1;
    }
    return nIRQ;
}

int rpi_irq_disable(uint32_t nIRQ) {

    uint32_t bank = RPI_IRQ_ID_BANK(nIRQ);
    uint32_t mask = RPI_IRQ_ID_MASK(nIRQ);

    if (bank == 0) {
        RPI_IRQ->DISABLE_1 = mask;

    } else if (bank == 1) {
        RPI_IRQ->DISABLE_2 = mask;

    } else if (bank == 2) {
        RPI_IRQ->DISABLE_BASIC = mask;
    } else {
        return -1;
    }

    return nIRQ;
}

void rpi_cpu_irq_enable(void) {
#ifdef RPI2
    __asm volatile (
            "CPSIE i \n"
            "DSB     \n"
            "ISB     \n " );
#else
    __asm volatile("CPSIE i" : : : "memory");
#endif
}

void rpi_cpu_irq_disable(void) {
#ifdef RPI2
    __asm volatile (
            "CPSID i \n"
            "DSB     \n"
            "ISB     \n " );
#else
    __asm volatile("CPSID i" : : : "memory");
#endif
}

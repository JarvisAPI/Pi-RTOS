#ifndef DRIVERS_RPI_CORE_H
#define DRIVERS_RPI_CORE_H

#include <stdint.h>
#include "Drivers/rpi_memory.h"

#define NUM_CORES 4
#define REG_BASE 0x40000000
#define MAILBOX_INT_OFFSET 0x50
#define CORE0_MAILBOX_INT_CNTRL (REG_BASE + 0x50)
#define CORE1_MAILBOX_INT_CNTRL (REG_BASE + 0x54)
#define CORE2_MAILBOX_INT_CNTRL (REG_BASE + 0x58)
#define CORE3_MAILBOX_INT_CNTRL (REG_BASE + 0x5C)

#define MAILBOX_SET_OFFSET 0x80
#define MAILBOX_OFFSET 0x4
#define CORE_OFFSET 0X10
#define CORE0_MAILBOX_0_SET (REG_BASE + 0x80)
#define CORE0_MAILBOX_1_SET (REG_BASE + 0x84)
#define CORE0_MAILBOX_2_SET (REG_BASE + 0x88)
#define CORE0_MAILBOX_3_SET (REG_BASE + 0x8C)

#define CORE1_MAILBOX_0_SET (REG_BASE + 0x90)
#define CORE1_MAILBOX_1_SET (REG_BASE + 0x94)
#define CORE1_MAILBOX_2_SET (REG_BASE + 0x98)
#define CORE1_MAILBOX_3_SET (REG_BASE + 0x9C)

#define CORE2_MAILBOX_0_SET (REG_BASE + 0xA0)
#define CORE2_MAILBOX_1_SET (REG_BASE + 0xA4)
#define CORE2_MAILBOX_2_SET (REG_BASE + 0xA8)
#define CORE2_MAILBOX_3_SET (REG_BASE + 0xAC)

#define CORE3_MAILBOX_0_SET (REG_BASE + 0xB0)
#define CORE3_MAILBOX_1_SET (REG_BASE + 0xB4)
#define CORE3_MAILBOX_2_SET (REG_BASE + 0xB8)
#define CORE3_MAILBOX_3_SET (REG_BASE + 0xBC)

#define MAILBOX_RDCLR_OFFSET 0xC0
#define CORE0_MAILBOX_0_RDCLR (REG_BASE + 0xC0)
#define CORE0_MAILBOX_1_RDCLR (REG_BASE + 0xC4)
#define CORE0_MAILBOX_2_RDCLR (REG_BASE + 0xC8)
#define CORE0_MAILBOX_3_RDCLR (REG_BASE + 0xCC)

#define CORE1_MAILBOX_0_RDCLR (REG_BASE + 0xD0)
#define CORE1_MAILBOX_1_RDCLR (REG_BASE + 0xD4)
#define CORE1_MAILBOX_2_RDCLR (REG_BASE + 0xD8)
#define CORE1_MAILBOX_3_RDCLR (REG_BASE + 0xDC)

#define CORE2_MAILBOX_0_RDCLR (REG_BASE + 0xE0)
#define CORE2_MAILBOX_1_RDCLR (REG_BASE + 0xE4)
#define CORE2_MAILBOX_2_RDCLR (REG_BASE + 0xE8)
#define CORE2_MAILBOX_3_RDCLR (REG_BASE + 0xEC)

#define CORE3_MAILBOX_0_RDCLR (REG_BASE + 0xF0)
#define CORE3_MAILBOX_1_RDCLR (REG_BASE + 0xF4)
#define CORE3_MAILBOX_2_RDCLR (REG_BASE + 0xF8)
#define CORE3_MAILBOX_3_RDCLR (REG_BASE + 0xFC)

#define INT_SOURCE_OFFSET 0x60
#define CORE0_INT_SOURCE (REG_BASE + 0x60)
#define CORE1_INT_SOURCE (REG_BASE + 0x64)
#define CORE2_INT_SOURCE (REG_BASE + 0x68)
#define CORE3_INT_SOURCE (REG_BASE + 0x6C)

typedef void (*core_start_fn_t)(void);

extern uint32_t CPUID(void);
void core_start( uint8_t core_num );
void core_set_start_fn( uint32_t cpuid, core_start_fn_t fn );
void smp_init(void);

void core_enable_mailbox_interrupt( uint32_t mailbox_num);
void core_clear_mailbox_interrupt( uint32_t mailbox_num );
void core_send_interrupt( uint32_t cpuid, uint32_t mailbox_num, uint32_t val );
/**
 * Gets the mailbox interrupt source as a 1-hot encoding.
 * For example, values in binary: 0101, mailbox 0 and 2 have interrupts pending.
 * Only the first 4bits of the returned value can be non-zero.
 * 
 * @return one hot encoding indicating source of mailbox interrupt.
 */
uint32_t core_get_mailbox_interrupt_source(void);
uint32_t core_read_mailbox( uint32_t mailbox_num );

#endif // DRIVERS_RPI_CORE_H


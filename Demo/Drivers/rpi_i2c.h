#ifndef __RPI_I2C_H__
#define __RPI_I2C_H__

#include "rpi_base.h"

#ifdef RPI2
#define BSC0_BASE     (PRI_BASE_ADDRESS + 0x804000)  // I2C controller
#else
#define BSC0_BASE     (PRI_BASE_ADDRESS + 0x205000)  // I2C controller
#endif

typedef struct {
    volatile uint32_t C;
    volatile uint32_t S;
    volatile uint32_t DLEN;
    volatile uint32_t A;
    volatile uint32_t FIFO;
    volatile uint32_t DIV;
    volatile uint32_t DEL;
    volatile uint32_t CLKT;
} BSC_t;

#define BSC0 ((volatile BSC_t*)BSC0_BASE)

#define BSC_FIFO_SIZE 16

#define BSC_C_I2CEN     (1 << 15)
#define BSC_C_INTR      (1 << 10)
#define BSC_C_INTT      (1 << 9)
#define BSC_C_INTD      (1 << 8)
#define BSC_C_ST        (1 << 7)
#define BSC_C_CLEAR     (1 << 4)
#define BSC_C_READ      1

#define CLEAR_FIFO      BSC_C_CLEAR
#define START_READ      BSC_C_I2CEN|BSC_C_ST|BSC_C_CLEAR|BSC_C_READ
#define START_WRITE     BSC_C_I2CEN|BSC_C_ST
 
#define BSC_S_CLKT  (1 << 9)
#define BSC_S_ERR     (1 << 8)
#define BSC_S_RXF     (1 << 7)
#define BSC_S_TXE     (1 << 6)
#define BSC_S_RXD     (1 << 5)
#define BSC_S_TXD     (1 << 4)
#define BSC_S_RXR     (1 << 3)
#define BSC_S_TXW     (1 << 2)
#define BSC_S_DONE    (1 << 1)
#define BSC_S_TA      1
 
#define CLEAR_STATUS    BSC_S_CLKT|BSC_S_ERR|BSC_S_DONE

typedef enum {
	I2C_CLOCK_100_kHz	= 2500,		///< 2500 = 10us = 100 kHz
	I2C_CLOCK_400_kHz	= 626,		///< 622 = 2.504us = 399.3610 kHz
	I2C_CLOCK_1666_kHz	= 150,		///< 150 = 60ns = 1.666 MHz (default at reset)
	I2C_CLOCK_1689_kHz	= 148,		///< 148 = 59ns = 1.689 MHz
} I2CClockDivider;

typedef enum {
	I2C_OK			= 0x00,		///< Success
	I2C_ERR_NACK 	= 0x01,		///< Received a NACK
	I2C_ERR_CLKT 	= 0x02,		///< Received Clock Stretch Timeout
	I2C_ERR_DATA 	= 0x04		///< Not all data is sent / received
} I2C_Status;

// I2C Function Prototypes
void rpi_i2c_init();
void wait_i2c_done();
I2C_Status rpi_i2c_set_reg(unsigned int dev_addr,
                           unsigned int reg_addr,
                           unsigned char value);
I2C_Status rpi_i2c_write(unsigned int dev_addr,
                         unsigned int reg_addr,
                         int len,
                         unsigned char *data);
I2C_Status rpi_i2c_read(unsigned int dev_addr,
                        unsigned int reg_addr,
                        int len,
                        unsigned char *data);
I2C_Status rpi_i2c_get_reg(unsigned int dev_addr,
                           unsigned int reg_addr,
                           unsigned char *result);

#endif

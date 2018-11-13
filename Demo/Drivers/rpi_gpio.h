#ifndef _RPI_GPIO_H_
#define _RPI_GPIO_H_

#include "rpi_base.h"

#define RPI_GPIO_BASE       ( PRI_BASE_ADDRESS + 0x200000 )

/* GPIO function selection */
typedef enum {
    GPIO_FSEL_IN,
    GPIO_FSEL_OUT,
    GPIO_FSEL_ALT5,
    GPIO_FSEL_ALT4,
    GPIO_FSEL_ALT0,
    GPIO_FSEL_ALT1,
    GPIO_FSEL_ALT2,
    GPIO_FSEL_ALT3,
} GPIO_FSEL_t;

typedef enum {
    GPIO_EV_STATUS = 1,
    GPIO_EV_RISING_EDGE = 2,
    GPIO_EV_FALLING_EDGE = 4,
    GPIO_EV_HIGH_LVL = 8,
    GPIO_EV_LOW_LVL = 16,
    GPIO_EV_ASYNC_RISING_EDGE = 32,
    GPIO_EV_ASYNC_FALLING_EDGE = 64
} GPIO_EV_SEL_t;

typedef struct {
    volatile uint32_t GPFSEL[6]; ///< Function selection registers.
    volatile uint32_t Reserved_1;
    volatile uint32_t GPSET[2];
    volatile uint32_t Reserved_2;
    volatile uint32_t GPCLR[2];
    volatile uint32_t Reserved_3;
    volatile uint32_t GPLEV[2];
    volatile uint32_t Reserved_4;
    volatile uint32_t GPEDS[2];
    volatile uint32_t Reserved_5;
    volatile uint32_t GPREN[2];
    volatile uint32_t Reserved_6;
    volatile uint32_t GPFEN[2];
    volatile uint32_t Reserved_7;
    volatile uint32_t GPHEN[2];
    volatile uint32_t Reserved_8;
    volatile uint32_t GPLEN[2];
    volatile uint32_t Reserved_9;
    volatile uint32_t GPAREN[2];
    volatile uint32_t Reserved_A;
    volatile uint32_t GPAFEN[2];
    volatile uint32_t Reserved_B;
    volatile uint32_t GPPUD[1];
    volatile uint32_t GPPUDCLK[2];
    //Ignoring the reserved and test bytes
} RPI_GPIO_t;

/* GPIO pin setup */
void rpi_gpio_sel_fun(uint32_t pin, uint32_t func);

/* Set GPIO output level */
void rpi_gpio_set_val(uint32_t pin, uint32_t val);

uint32_t rpi_gpio_get_val(uint32_t pin);

void rpi_gpio_toggle(uint32_t pin);

uint32_t rpi_gpio_ev_read_status(uint32_t pin, GPIO_EV_SEL_t event);
void rpi_gpio_ev_clear_status(uint32_t pin);
void rpi_gpio_ev_detect_enable(uint32_t pin, GPIO_EV_SEL_t events);
void rpi_gpio_ev_detect_disable(uint32_t pin, GPIO_EV_SEL_t events);


#define RPI_GPIO ((volatile RPI_GPIO_t *)RPI_GPIO_BASE)

#endif /* _RPI_GPIO_H_ */

/*
 * rpio_gpio.c
 *
 *  Created on: Mar 7, 2016
 *      Author: TsungMin Huang
 */

#include <stdint.h>

#include "rpi_gpio.h"

void rpi_gpio_sel_fun(uint32_t pin, uint32_t func) {
    int offset = pin / 10;
    uint32_t val = RPI_GPIO->GPFSEL[offset];// Read in the original register value.

    int item = pin % 10;
    val &= ~(0x7 << (item * 3));
    val |= ((func & 0x7) << (item * 3));
    RPI_GPIO->GPFSEL[offset] = val;
}

void rpi_gpio_set_val(uint32_t pin, uint32_t val) {
    uint32_t offset = pin / 32;
    uint32_t mask = (1 << (pin % 32));

    if (val) {
        RPI_GPIO->GPSET[offset] |= mask;
    } else {
        RPI_GPIO->GPCLR[offset] |= mask;
    }
}

uint32_t rpi_gpio_get_val(uint32_t pin) {
    uint32_t offset = pin / 32;
    uint32_t mask = (1 <<(pin % 32));
    uint32_t result = RPI_GPIO->GPLEV[offset] & mask;

    return result ? 1 : 0;
}

void rpi_gpio_toggle(uint32_t pin) {

    uint32_t result = rpi_gpio_get_val(pin);
    if(result) {
        rpi_gpio_set_val(pin, 0);
    } else {
        rpi_gpio_set_val(pin, 1);
    }
}

uint32_t rpi_gpio_ev_read_status(uint32_t pin, GPIO_EV_SEL_t event){
    uint32_t offset = pin / 32;
    uint32_t mask = (1 <<(pin % 32));
    uint32_t result = 0;

    if(event & GPIO_EV_STATUS){
        result = RPI_GPIO->GPEDS[offset];
    }

    if(event & GPIO_EV_RISING_EDGE){
        result = RPI_GPIO->GPREN[offset];
    }

    if(event & GPIO_EV_FALLING_EDGE){
        result = RPI_GPIO->GPFEN[offset];
    }

    if(event & GPIO_EV_HIGH_LVL){
        result = RPI_GPIO->GPHEN[offset];
    }

    if(event & GPIO_EV_LOW_LVL){
        result = RPI_GPIO->GPLEN[offset];
    }

    if(event & GPIO_EV_ASYNC_RISING_EDGE){
        result = RPI_GPIO->GPAREN[offset];
    }

    if(event & GPIO_EV_ASYNC_FALLING_EDGE){
        result = RPI_GPIO->GPAFEN[offset];
    }

    return result & mask ? 1 : 0;
}

void rpi_gpio_ev_clear_status(uint32_t pin){
    uint32_t offset = pin / 32;
    uint32_t mask = (1 <<(pin % 32));

    RPI_GPIO->GPEDS[offset] |= mask;
}

void rpi_gpio_ev_detect_enable(uint32_t pin, GPIO_EV_SEL_t events){
    uint32_t offset = pin / 32;
    uint32_t mask = (1 <<(pin % 32));

    if(events & GPIO_EV_STATUS){
        RPI_GPIO->GPEDS[offset] |= mask;
    }

    if(events & GPIO_EV_RISING_EDGE){
        RPI_GPIO->GPREN[offset] |= mask;
    }

    if(events & GPIO_EV_FALLING_EDGE){
        RPI_GPIO->GPFEN[offset] |= mask;
    }

    if(events & GPIO_EV_HIGH_LVL){
        RPI_GPIO->GPHEN[offset] |= mask;
    }

    if(events & GPIO_EV_LOW_LVL){
        RPI_GPIO->GPLEN[offset] |= mask;
    }

    if(events & GPIO_EV_ASYNC_RISING_EDGE){
        RPI_GPIO->GPAREN[offset] |= mask;
    }

    if(events & GPIO_EV_ASYNC_FALLING_EDGE){
        RPI_GPIO->GPAFEN[offset] |= mask;
    }
}

void rpi_gpio_ev_detect_disable(uint32_t pin, GPIO_EV_SEL_t events){
    uint32_t offset = pin / 32;
    uint32_t mask = (1 <<(pin % 32));

    if(events & GPIO_EV_STATUS){
        RPI_GPIO->GPEDS[offset] &= ~mask;
    }

    if(events & GPIO_EV_RISING_EDGE){
        RPI_GPIO->GPREN[offset] &= ~mask;
    }

    if(events & GPIO_EV_FALLING_EDGE){
        RPI_GPIO->GPFEN[offset] &= ~mask;
    }

    if(events & GPIO_EV_HIGH_LVL){
        RPI_GPIO->GPHEN[offset] &= ~mask;
    }

    if(events & GPIO_EV_LOW_LVL){
        RPI_GPIO->GPLEN[offset] &= ~mask;
    }

    if(events & GPIO_EV_ASYNC_RISING_EDGE){
        RPI_GPIO->GPAREN[offset] &= ~mask;
    }

    if(events & GPIO_EV_ASYNC_FALLING_EDGE){
        RPI_GPIO->GPAFEN[offset] &= ~mask;
    }
}

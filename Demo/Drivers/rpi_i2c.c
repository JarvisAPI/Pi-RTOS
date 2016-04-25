#include "rpi_i2c.h"
#include "rpi_gpio.h"

// Initialize I2C
void rpi_i2c_init()
{
    rpi_gpio_sel_fun(2, GPIO_FSEL_IN);
    rpi_gpio_sel_fun(3, GPIO_FSEL_IN);
    
    rpi_gpio_sel_fun(2, GPIO_FSEL_ALT0);
    rpi_gpio_sel_fun(3, GPIO_FSEL_ALT0);
    
    BSC0->DIV = I2C_CLOCK_400_kHz;
}  
 
// Function to wait for the I2C transaction to complete
void wait_i2c_done()
{
    while(1){
        if(!(BSC0->S & BSC_S_DONE)) {
            break;
        } 
    }
}

I2C_Status rpi_i2c_write(unsigned int dev_addr,
                   unsigned int reg_addr,
                   int len,
                   unsigned char *data)
{
    int i = -1;
    int rem = len + 1;
    I2C_Status res = I2C_OK;
    unsigned int status = 0;
    
    BSC0->C = CLEAR_FIFO;
    BSC0->S = CLEAR_STATUS;
    BSC0->A = dev_addr & 0x7F;
    BSC0->DLEN = len+1;
    while ((0 != rem)  && (i < BSC_FIFO_SIZE)) {
		BSC0->FIFO = i < 0 ? reg_addr : *(data+i);
		++i;
		--rem;
	}

    BSC0->C = START_WRITE;

    while (!(BSC0->S & BSC_S_DONE)) {
		while ((0 != rem) && (BSC0->S & BSC_S_TXD)) {
			BSC0->FIFO = i < 0 ? reg_addr : *(data+i);
			++i;
			--rem;
    	}
    }


    status = BSC0->S;
    if(status & BSC_S_ERR){
        BSC0->S = BSC_S_ERR;
        res = I2C_ERR_NACK;
    } else if (status & BSC_S_CLKT){
        res = I2C_ERR_CLKT;
    } else if (0 != rem) {
        res = I2C_ERR_DATA;
    }

    BSC0->S = BSC_S_DONE;

    return res;
}

I2C_Status rpi_i2c_set_reg(unsigned int dev_addr,
                     unsigned int reg_addr,
                     unsigned char value)
{
    return rpi_i2c_write(dev_addr, reg_addr, 1, &value);
}

I2C_Status _read(unsigned char *data,
                 int len)
{
    int rem = len;
    int i = 0;
    I2C_Status res = I2C_OK;
    unsigned int status = 0;

    BSC0->C = CLEAR_FIFO;
    BSC0->S = CLEAR_STATUS;
    BSC0->DLEN = len;
    BSC0->C = START_READ;

    while (!(BSC0->S & BSC_S_DONE)){
        while(BSC0->S & BSC_S_RXD){
            *(data+i) = (unsigned char)(BSC0->FIFO & 0xFF);
            ++i;
            --rem;
        }
    }
    while ((0 != rem) && (BSC0->S & BSC_S_RXD)){
        *(data+i) = (unsigned char)(BSC0->FIFO & 0xFF);
        ++i;
        --rem;
    }

    status = BSC0->S;
    if(status & BSC_S_ERR){
        BSC0->S = BSC_S_ERR;
        res = I2C_ERR_NACK;
    } else if (status & BSC_S_CLKT){
        res = I2C_ERR_CLKT;
    } else if (0 != rem) {
        res = I2C_ERR_DATA;
    }

    BSC0->S = BSC_S_DONE;
    
    return res;
}

I2C_Status rpi_i2c_read(unsigned int dev_addr,
                  unsigned int reg_addr,
                  int len,
                  unsigned char *data)
{
    I2C_Status res = rpi_i2c_write(dev_addr, reg_addr, 0, 0);
    if(I2C_OK == res){
        res = _read(data, len);
    }

    return res;
}

I2C_Status rpi_i2c_get_reg(unsigned int dev_addr,
                           unsigned int reg_addr,
                           unsigned char *result)
{
    return rpi_i2c_read(dev_addr, reg_addr, 1, result);
}

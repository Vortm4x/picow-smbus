#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <pico/stdlib.h>
#include <pico/cyw43_arch.h>
#include <hardware/i2c.h>
#include <smbus/smbus_slave.h>
#include "handlers.h"

#define PICO_SMBUS_SLAVE_I2C_INSTANCE    i2c0
#define PICO_SMBUS_SLAVE_I2C_ADDRESS     0x17
#define PICO_SMBUS_SLAVE_BAUDRATE        100000
#define PICO_SMBUS_SLAVE_SMDAT_PIN       12
#define PICO_SMBUS_SLAVE_SMCLK_PIN       13

static void pico_smbus_slave_init();
static bool init_all();

void pico_smbus_slave_init()
{
    smbus_slave_init(
        PICO_SMBUS_SLAVE_I2C_INSTANCE, 
        PICO_SMBUS_SLAVE_I2C_ADDRESS, 
        PICO_SMBUS_SLAVE_BAUDRATE,
        PICO_SMBUS_SLAVE_SMDAT_PIN,
        PICO_SMBUS_SLAVE_SMCLK_PIN
    );

    smbus_set_quick_handler(PICO_SMBUS_SLAVE_I2C_INSTANCE, quick_handler);
    smbus_set_write_reg_handler(PICO_SMBUS_SLAVE_I2C_INSTANCE, write_reg_handler);
    smbus_set_write_data_handler(PICO_SMBUS_SLAVE_I2C_INSTANCE, write_data_handler);
    smbus_set_read_reg_handler(PICO_SMBUS_SLAVE_I2C_INSTANCE, read_reg_handler);
    smbus_set_read_data_handler(PICO_SMBUS_SLAVE_I2C_INSTANCE, read_data_handler);
    smbus_set_proc_call_handler(PICO_SMBUS_SLAVE_I2C_INSTANCE, proc_call_handler);

    smbus_set_pec(PICO_SMBUS_SLAVE_I2C_INSTANCE, true);
}


bool init_all()
{
    if(!stdio_init_all())
    {
        printf("stdio init failed\n");
        return false;
    }
    
    if (cyw43_arch_init() != PICO_OK) 
    {
        printf("CYW43 arch init failed\n");
        return false;
    }

    pico_smbus_slave_init();

    return true;
}


int main() 
{   
    if(!init_all())
    {
        return -1;
    }

    printf("Pico SMBUS slave started at 0x%02X\n", PICO_SMBUS_SLAVE_I2C_ADDRESS);

    while (true);
    
    return 0;
}
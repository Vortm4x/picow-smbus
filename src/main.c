#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <pico/stdlib.h>
#include <pico/cyw43_arch.h>
#include <hardware/gpio.h>
#include <hardware/i2c.h>

#include <pico/i2c_slave.h>
#include "smbus_slave.h"


#define PICO_I2C_SLAVE_INSTANCE        i2c0
#define PICO_I2C_SLAVE_SDA_PIN         12
#define PICO_I2C_SLAVE_SCL_PIN         13
#define PICO_I2C_SLAVE_ADDRESS         0x17
#define PICO_I2C_SLAVE_BAUDRATE        100000


void quick_handler(bool value);
void read_byte_handler(uint8_t* byte);
void undo_read_byte_handler();
void pico_smbus_slave_init();
bool init_all();


void quick_handler(bool value)
{
    if(value)
    {
        printf("Quick ON\n");
    }
    else
    {
        printf("Quick OFF\n");
    }
}

void read_byte_handler(uint8_t* byte)
{
    *byte = 0xFC; 
    printf("Read byte\n");
}

void undo_read_byte_handler()
{
    printf("Undo Read Byte\n");
}


void pico_smbus_slave_init()
{
    gpio_init(PICO_I2C_SLAVE_SDA_PIN);
    gpio_set_function(PICO_I2C_SLAVE_SDA_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(PICO_I2C_SLAVE_SDA_PIN);

    gpio_init(PICO_I2C_SLAVE_SCL_PIN);
    gpio_set_function(PICO_I2C_SLAVE_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(PICO_I2C_SLAVE_SCL_PIN);

    smbus_slave_init(
        PICO_I2C_SLAVE_INSTANCE, 
        PICO_I2C_SLAVE_ADDRESS, 
        PICO_I2C_SLAVE_BAUDRATE
    );

    smbus_set_quick_handler(PICO_I2C_SLAVE_INSTANCE, quick_handler);
    smbus_set_read_byte_handler(PICO_I2C_SLAVE_INSTANCE, read_byte_handler);
    smbus_set_undo_read_byte_handler(PICO_I2C_SLAVE_INSTANCE, undo_read_byte_handler);
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

    printf("Pico SMBUS slave started at 0x%02X\n", PICO_I2C_SLAVE_ADDRESS);

    while (true);
    
    return 0;
}
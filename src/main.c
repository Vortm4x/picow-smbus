#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <pico/stdlib.h>
#include <pico/cyw43_arch.h>
#include <hardware/gpio.h>
#include <hardware/i2c.h>

#include <pico/i2c_slave.h>
#include "smbus_slave.h"


#define PICO_SMBUS_SLAVE_I2C_INSTANCE    i2c0
#define PICO_SMBUS_SLAVE_I2C_ADDRESS     0x17
#define PICO_SMBUS_SLAVE_BAUDRATE        50000
#define PICO_SMBUS_SLAVE_SMDAT_PIN       12
#define PICO_SMBUS_SLAVE_SMCLK_PIN       13

#define SMBUS_BYTE_CMD 0xC0
#define SMBUS_BYTE_DATA_CMD 0xC1
#define SMBUS_WORD_DATA_CMD 0xC2
#define SMBUS_BLOCK_DATA_CMD 0xCB


static void quick_handler(bool value);
static void read_byte_handler(uint8_t* byte);
static void write_byte_handler(uint8_t* byte);
static void read_data_handler(uint8_t command, smbus_data_t* smbus_data);
static void write_data_handler(uint8_t command, smbus_data_t* smbus_data);
static void proc_call_handler(uint8_t command, uint16_t* request, uint16_t* response);

static void pico_smbus_slave_init();
static bool init_all();


// #define SMBUS_PICO_DBG

#ifdef SMBUS_PICO_DBG
    #define PICO_PRINT(...) printf(__VA_ARGS__)
#else
    #define PICO_PRINT(...)
#endif


void quick_handler(bool value)
{
    if(value)
    {
        PICO_PRINT("QUICK ON\n");
    }
    else
    {
        PICO_PRINT("QUICK OfF\n");
    }
}

void read_byte_handler(uint8_t* byte)
{
    *byte = SMBUS_BYTE_CMD;
    PICO_PRINT("[TX] byte: 0x%02X\n", *byte);
}

void write_byte_handler(uint8_t* byte)
{
    PICO_PRINT("[RX] byte: 0x%02X\n", *byte);
}

void read_data_handler(uint8_t command, smbus_data_t* smbus_data)
{
    switch (command)
    {
        case SMBUS_BYTE_DATA_CMD:
        {
            smbus_data->byte = 0x11;
            PICO_PRINT("[TX] byte data: 0x%02X\n", smbus_data->byte);
        }
        break;
    
        case SMBUS_WORD_DATA_CMD:
        {
            smbus_data->word = 0x1122;
            PICO_PRINT("[TX] word data: 0x%04X\n", smbus_data->word);
        }
        break;

        case SMBUS_BLOCK_DATA_CMD:
        {
            PICO_PRINT("[TX] block data:");

            for(int i = 0; i < SMBUS_MAX_BLOCK_LEN; ++i)
            {
                smbus_data->block[i] = i;
                PICO_PRINT(" 0x%02X", smbus_data->block[i]);
            }

            PICO_PRINT("\n");
        }
        break;
    }   
}

void write_data_handler(uint8_t command, smbus_data_t* smbus_data)
{
    switch (command)
    {
        case SMBUS_BYTE_DATA_CMD:
        {
            PICO_PRINT("[RX] byte data: 0x%02X\n", smbus_data->byte);
        }
        break;
    
        case SMBUS_WORD_DATA_CMD:
        {
            PICO_PRINT("[RX] word data: 0x%04X\n", smbus_data->word);
        }
        break;

        case SMBUS_BLOCK_DATA_CMD:
        {
            PICO_PRINT("[RX] block data:");

            for(int i = 0; i < SMBUS_MAX_BLOCK_LEN; ++i)
            {
                PICO_PRINT(" 0x%02X", smbus_data->block[i]);
            }
            
            PICO_PRINT("\n");
        }
        break;
    }   
}

void proc_call_handler(uint8_t command, uint16_t* request, uint16_t* response)
{
    *response = 0x8246;
    PICO_PRINT("[RROC] command: 0x%02X, req: 0x%04X, resp: 0x%04X\n", command, *request, *response);
}


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
    smbus_set_read_byte_handler(PICO_SMBUS_SLAVE_I2C_INSTANCE, read_byte_handler);
    smbus_set_write_byte_handler(PICO_SMBUS_SLAVE_I2C_INSTANCE, write_byte_handler);
    smbus_set_read_data_handler(PICO_SMBUS_SLAVE_I2C_INSTANCE, read_data_handler);
    smbus_set_write_data_handler(PICO_SMBUS_SLAVE_I2C_INSTANCE, write_data_handler);
    smbus_set_proc_call_handler(PICO_SMBUS_SLAVE_I2C_INSTANCE, proc_call_handler);
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
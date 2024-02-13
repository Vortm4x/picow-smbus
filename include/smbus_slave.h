#ifndef PICO_SMBUS_SLAVE_H
#define PICO_SMBUS_SLAVE_H

#include <hardware/i2c.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SMBUS_MAX_BLOCK_LEN 32

typedef enum smbus_slave_event_t
{
    SMBUS_SLAVE_QUICK,
    SMBUS_SLAVE_READ_BYTE, 
    SMBUS_SLAVE_WRITE_BYTE,
    SMBUS_SLAVE_READ_DATA,
    SMBUS_SLAVE_WRITE_DATA,
    SMBUS_SLAVE_PROC_CALL,
}
smbus_slave_event_t;


typedef union 
{
    uint8_t byte;
    uint16_t word;
    uint8_t block[SMBUS_MAX_BLOCK_LEN + 1];
}
smbus_data_t;

typedef void (*data_handler_t)(uint8_t command, smbus_data_t* smbus_data);
typedef void (*byte_handler_t)(uint8_t* byte);
typedef void (*quick_handler_t)(bool value);
typedef void (*proc_call_handler_t)(uint8_t command, uint16_t* request, uint16_t* response);


void smbus_slave_init(
    i2c_inst_t* i2c, 
    uint8_t address, 
    uint baudrate,
    uint sda_pin,
    uint scl_pin
);

void smbus_slave_deinit(
    i2c_inst_t* i2c
);

void smbus_set_quick_handler(i2c_inst_t* i2c, quick_handler_t quick_handler);
void smbus_set_read_byte_handler(i2c_inst_t* i2c, byte_handler_t byte_handler);
void smbus_set_write_byte_handler(i2c_inst_t* i2c, byte_handler_t byte_handler);
void smbus_set_read_data_handler(i2c_inst_t* i2c, data_handler_t data_handler);
void smbus_set_write_data_handler(i2c_inst_t* i2c, data_handler_t data_handler);
void smbus_set_proc_call_handler(i2c_inst_t* i2c, proc_call_handler_t proc_call_handler);

void smbus_reset_handler(i2c_inst_t* i2c, smbus_slave_event_t slave_event);


#ifdef __cplusplus
}
#endif

#endif
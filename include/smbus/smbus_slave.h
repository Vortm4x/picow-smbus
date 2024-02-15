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
    SMBUS_SLAVE_WRITE_REG,
    SMBUS_SLAVE_WRITE_DATA,
    SMBUS_SLAVE_READ_REG, 
    SMBUS_SLAVE_READ_DATA,
    SMBUS_SLAVE_PROC_CALL,
}
smbus_slave_event_t;

typedef union 
{
    uint8_t byte;
    uint16_t word;
    uint32_t dword;
    uint64_t qword;
    uint8_t block[SMBUS_MAX_BLOCK_LEN + 2];
}
smbus_data_t;


typedef void (*quick_handler_t)(bool is_on);
typedef void (*write_reg_handler_t)(uint8_t reg);
typedef void (*write_data_handler_t)(uint8_t command, const smbus_data_t* smbus_data);
typedef uint8_t (*read_reg_handler_t)();
typedef size_t (*read_data_handler_t)(uint8_t command, smbus_data_t* smbus_data);
typedef uint16_t (*proc_call_handler_t)(uint8_t command, uint16_t request);


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


void smbus_set_quick_handler(i2c_inst_t* i2c, quick_handler_t handler);
void smbus_set_write_reg_handler(i2c_inst_t* i2c, write_reg_handler_t handler);
void smbus_set_write_data_handler(i2c_inst_t* i2c, write_data_handler_t handler);
void smbus_set_read_reg_handler(i2c_inst_t* i2c, read_reg_handler_t handler);
void smbus_set_read_data_handler(i2c_inst_t* i2c, read_data_handler_t handler);
void smbus_set_proc_call_handler(i2c_inst_t* i2c, proc_call_handler_t handler);

void smbus_reset_handler(i2c_inst_t* i2c, smbus_slave_event_t slave_event);

void smbus_set_pec(i2c_inst_t* i2c, bool is_enabled);
bool smbus_get_pec(i2c_inst_t* i2c);


#ifdef __cplusplus
}
#endif

#endif
#ifndef HANDLERS_H
#define HANDLERS_H

#include <smbus/smbus_slave.h>

#define PICO_SMBUS_DEBUG

void quick_handler(bool is_on);
void write_reg_handler(uint8_t reg);
void write_data_handler(uint8_t command, const smbus_data_t* smbus_data);
uint8_t read_reg_handler();
size_t read_data_handler(uint8_t command, smbus_data_t* smbus_data);
uint16_t proc_call_handler(uint8_t command, uint16_t request);

#endif // HANDLERS_H
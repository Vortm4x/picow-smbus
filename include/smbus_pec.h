#ifndef SMBUS_PEC_H
#define SMBUS_PEC_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

uint8_t smbus_pec_single(uint8_t crc, uint8_t data);
uint8_t smbus_pec_block(uint8_t crc, uint8_t block[], size_t block_len);

#endif // SMBUS_PEC_H
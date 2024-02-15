#ifndef COMMANDS_H
#define COMMANDS_H

// SMBUS TEST COMMANDS LIST

#define SMBUS_CMD_REG               0xC0
#define SMBUS_CMD_BYTE_DATA         0xC1
#define SMBUS_CMD_WORD_DATA         0xC2
#define SMBUS_CMD_DWORD_DATA        0xC3
#define SMBUS_CMD_QWORD_DATA        0xC4
#define SMBUS_CMD_BLOCK_DATA        0xCB
#define SMBUS_CMD_PROC_CALL         0xCC

#endif // COMMANDS_H
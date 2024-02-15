#include "handlers.h"
#include "commands.h"
#include <pico/stdio.h>


#ifdef PICO_SMBUS_DEBUG

    static const char hex_alpla[] = { 
        '0', '1', '2', '3', 
        '4', '5', '6', '7', 
        '8', '9', 'A', 'B', 
        'C', 'D', 'E', 'F' 
    };

    #define PICO_PUTCHAR(x) \
        putchar_raw(x)

    #define PICO_PUTBYTE(x)                                     \
        do                                                      \
        {                                                       \
            uint8_t half_byte;                                  \
                                                                \
            half_byte = ((x >> 4) & 0xF);                       \
            PICO_PUTCHAR(hex_alpla[half_byte]);                 \
                                                                \
            half_byte = ((x >> 0) & 0xF);                       \
            PICO_PUTCHAR(hex_alpla[half_byte]);                 \
        }                                                       \
        while (0)

#else

    #define PICO_PUTCHAR(x)
    #define PICO_PUTBYTE(x)

#endif // PICO_SMBUS_DEBUG



void quick_handler(bool is_on)
{
    PICO_PUTCHAR('Q');
    PICO_PUTCHAR('-');

    if(is_on)
    {
        PICO_PUTCHAR('O');
        PICO_PUTCHAR('N');
    }
    else
    {
        PICO_PUTCHAR('O');
        PICO_PUTCHAR('F');
        PICO_PUTCHAR('F');
    }

    PICO_PUTCHAR('\n');
}

void write_reg_handler(uint8_t reg)
{
    PICO_PUTCHAR('W');
    PICO_PUTCHAR('-');
    PICO_PUTBYTE(reg);

    PICO_PUTCHAR('\n');
}

void write_data_handler(uint8_t command, const smbus_data_t* smbus_data)
{
    PICO_PUTCHAR('W');
    PICO_PUTCHAR('-');
    PICO_PUTBYTE(command);

    PICO_PUTCHAR(' ');

    switch (command)
    {
        case SMBUS_CMD_BYTE_DATA:
        {
            PICO_PUTBYTE(smbus_data->byte);
        }
        break;

        case SMBUS_CMD_WORD_DATA:
        {
            PICO_PUTBYTE((uint8_t)(smbus_data->word >> 8));
            PICO_PUTBYTE((uint8_t)(smbus_data->word >> 0));
        }
        break;

        case SMBUS_CMD_DWORD_DATA:
        {
            PICO_PUTBYTE((uint8_t)(smbus_data->dword >> 24));
            PICO_PUTBYTE((uint8_t)(smbus_data->dword >> 16));
            PICO_PUTBYTE((uint8_t)(smbus_data->dword >> 8));
            PICO_PUTBYTE((uint8_t)(smbus_data->dword >> 0));
        }
        break;

        case SMBUS_CMD_QWORD_DATA:
        {
            PICO_PUTBYTE((uint8_t)(smbus_data->qword >> 56));
            PICO_PUTBYTE((uint8_t)(smbus_data->qword >> 48));
            PICO_PUTBYTE((uint8_t)(smbus_data->qword >> 40));
            PICO_PUTBYTE((uint8_t)(smbus_data->qword >> 32));
            PICO_PUTBYTE((uint8_t)(smbus_data->qword >> 24));
            PICO_PUTBYTE((uint8_t)(smbus_data->qword >> 16));
            PICO_PUTBYTE((uint8_t)(smbus_data->qword >> 8));
            PICO_PUTBYTE((uint8_t)(smbus_data->qword >> 0));
        }
        break;

        case SMBUS_CMD_BLOCK_DATA:
        {
            PICO_PUTBYTE(smbus_data->block[0]);

            for (uint8_t i = 0; i < smbus_data->block[0]; ++i)
            {
                PICO_PUTCHAR(' ');
                PICO_PUTBYTE(smbus_data->block[i + 1]);
            }
        }
        break;
    }

    PICO_PUTCHAR('\n');
}

uint8_t read_reg_handler()
{
    uint8_t reg = SMBUS_CMD_REG;

    PICO_PUTCHAR('R');
    PICO_PUTCHAR('-');
    PICO_PUTBYTE(reg);

    PICO_PUTCHAR('\n');

    return reg;
}

size_t read_data_handler(uint8_t command, smbus_data_t* smbus_data)
{
    size_t size = 0;

    PICO_PUTCHAR('R');
    PICO_PUTCHAR('-');
    PICO_PUTBYTE(command);

    PICO_PUTCHAR(' ');

    switch (command)
    {
        case SMBUS_CMD_BYTE_DATA:
        {
            smbus_data->byte = 0x01;
            size = sizeof(uint8_t);

            PICO_PUTBYTE(smbus_data->byte);
        }
        break;

        case SMBUS_CMD_WORD_DATA:
        {
            smbus_data->word = 0x0123;
            size = sizeof(uint16_t);

            PICO_PUTBYTE((uint8_t)(smbus_data->word >> 8));
            PICO_PUTBYTE((uint8_t)(smbus_data->word >> 0));
        }
        break;

        case SMBUS_CMD_DWORD_DATA:
        {
            smbus_data->dword = 0x01234567;
            size = sizeof(uint32_t);

            PICO_PUTBYTE((uint8_t)(smbus_data->dword >> 24));
            PICO_PUTBYTE((uint8_t)(smbus_data->dword >> 16));
            PICO_PUTBYTE((uint8_t)(smbus_data->dword >> 8));
            PICO_PUTBYTE((uint8_t)(smbus_data->dword >> 0));
        }
        break;

        case SMBUS_CMD_QWORD_DATA:
        {
            smbus_data->qword = 0x0123456789ABCDEF;
            size = sizeof(uint64_t);

            PICO_PUTBYTE((uint8_t)(smbus_data->qword >> 56));
            PICO_PUTBYTE((uint8_t)(smbus_data->qword >> 48));
            PICO_PUTBYTE((uint8_t)(smbus_data->qword >> 40));
            PICO_PUTBYTE((uint8_t)(smbus_data->qword >> 32));
            PICO_PUTBYTE((uint8_t)(smbus_data->qword >> 24));
            PICO_PUTBYTE((uint8_t)(smbus_data->qword >> 16));
            PICO_PUTBYTE((uint8_t)(smbus_data->qword >> 8));
            PICO_PUTBYTE((uint8_t)(smbus_data->qword >> 0));
        }
        break;

        case SMBUS_CMD_BLOCK_DATA:
        {
            smbus_data->block[0] = SMBUS_MAX_BLOCK_LEN;
            PICO_PUTBYTE(smbus_data->block[0]);

            for (uint8_t i = 0; i < smbus_data->block[0]; ++i)
            {
                smbus_data->block[i + 1] = 0xA0 | i;

                PICO_PUTCHAR(' ');
                PICO_PUTBYTE(smbus_data->block[i + 1]);
            }

            size = smbus_data->block[0] + 1;
        }
        break;
    }

    PICO_PUTCHAR('\n');

    return size;
}

uint16_t proc_call_handler(uint8_t command, uint16_t request)
{
    uint16_t response = 0x8246;

    PICO_PUTCHAR('C');
    PICO_PUTCHAR('-');
    PICO_PUTBYTE(command);

    PICO_PUTCHAR(' ');
    PICO_PUTCHAR('W');
    PICO_PUTCHAR(' ');
    PICO_PUTBYTE((uint8_t)(request >> 8));
    PICO_PUTBYTE((uint8_t)(request >> 0));
    
    PICO_PUTCHAR(' ');
    PICO_PUTCHAR('R');
    PICO_PUTCHAR(' ');
    PICO_PUTBYTE((uint8_t)(response >> 8));
    PICO_PUTBYTE((uint8_t)(response >> 0));

    PICO_PUTCHAR('\n');

    return response;
}
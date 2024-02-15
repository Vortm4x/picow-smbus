#include <smbus/smbus_slave.h>
#include <hardware/irq.h>
#include <hardware/gpio.h> 
#include <smbus_pec.h>
#include <string.h>

#include <stdio.h>

#define SMBUS_MIN_BAUD_RATE_HZ _u(10000)
#define SMBUS_MAX_BAUD_RATE_HZ _u(100000)

typedef struct smbus_slave_t
{
    quick_handler_t quick_handler;
    write_reg_handler_t write_reg_handler;
    write_data_handler_t write_data_handler;
    read_reg_handler_t read_reg_handler;
    read_data_handler_t read_data_handler;
    proc_call_handler_t proc_call_handler;
    bool is_pec_enabled;

    uint scl_pin;
    uint sda_pin;
    
    bool is_cmd_received;
    bool is_cmd_sent;
    bool is_restarted;
    bool is_quick_on;
    
    uint8_t cmd_byte;
    smbus_data_t smbus_data;
    uint8_t io_next_byte;
}
smbus_slave_t;

static smbus_slave_t smbus_slaves[2];

static void __isr __not_in_flash_func(smbus_slave_irq_restart)(uint bus_index);
static void __isr __not_in_flash_func(smbus_slave_irq_start)(uint bus_index);
static void __isr __not_in_flash_func(smbus_slave_irq_stop)(uint bus_index);
static void __isr __not_in_flash_func(smbus_slave_irq_tx_abrt)(uint bus_index);
static void __isr __not_in_flash_func(smbus_slave_irq_rx_full)(uint bus_index);
static void __isr __not_in_flash_func(smbus_slave_irq_rd_req)(uint bus_index);
static void __isr __not_in_flash_func(smbus_slave_irq_handler)(void);

static void smbus_init_i2c_gpio(uint gpio);
static uint8_t smbus_get_unshifted_address(uint bus_index, bool readwrite_bit);


void smbus_slave_irq_restart(uint bus_index)
{
    smbus_slave_t* slave = &smbus_slaves[bus_index];

    if(slave->io_next_byte == 0)
    {
        if(slave->read_data_handler != NULL)
        {
            size_t data_len = slave->read_data_handler(slave->cmd_byte, &slave->smbus_data);

            if(slave->is_pec_enabled)
            {
                uint8_t read_address = smbus_get_unshifted_address(bus_index, true);
                uint8_t write_address = smbus_get_unshifted_address(bus_index, false);
                uint8_t crc = 0;

                crc = smbus_pec_single(crc, write_address);
                crc = smbus_pec_single(crc, slave->cmd_byte);
                crc = smbus_pec_single(crc, read_address);
                crc = smbus_pec_block(crc, slave->smbus_data.block, data_len);

                slave->smbus_data.block[data_len] = crc;
            }
        }
    }
    else
    if(slave->io_next_byte == 2)
    {
        if(slave->proc_call_handler != NULL)
        {
            uint16_t request = slave->smbus_data.word;
            uint16_t response = slave->proc_call_handler(slave->cmd_byte, request);
            
            if(slave->is_pec_enabled)
            {
                uint8_t read_address = smbus_get_unshifted_address(bus_index, true);
                uint8_t write_address = smbus_get_unshifted_address(bus_index, false);
                uint8_t crc = 0;

                crc = smbus_pec_single(crc, write_address);
                crc = smbus_pec_single(crc, slave->cmd_byte);
                crc = smbus_pec_block(crc, slave->smbus_data.block, slave->io_next_byte);
                slave->smbus_data.word = response;
                crc = smbus_pec_single(crc, read_address);
                crc = smbus_pec_block(crc, slave->smbus_data.block, slave->io_next_byte);

                slave->smbus_data.block[slave->io_next_byte] = crc;
            }
            else
            {
                slave->smbus_data.word = response;
            }

            slave->io_next_byte = 0;
        }
    }

    slave->is_restarted = true;
}

void smbus_slave_irq_start(uint bus_index)
{}

void smbus_slave_irq_tx_abrt(uint bus_index)
{}

void smbus_slave_irq_stop(uint bus_index)
{    
    smbus_slave_t* slave = &smbus_slaves[bus_index];

    if(slave->is_cmd_received && !slave->is_restarted)
    {
        bool allow_write = true;

        if(slave->is_pec_enabled)
        {
            if(slave->io_next_byte > 0)
            {
                uint8_t write_address = smbus_get_unshifted_address(bus_index, false);
                uint8_t crc = 0;
                
                crc = smbus_pec_single(crc, write_address);
                crc = smbus_pec_single(crc, slave->cmd_byte);

                slave->io_next_byte -= 1;

                if(slave->io_next_byte > 0)
                {
                    crc = smbus_pec_block(crc, slave->smbus_data.block, slave->io_next_byte);
                }

                allow_write = (crc == slave->smbus_data.block[slave->io_next_byte]);            
            }
            else
            {
                allow_write = false;
            }
        }

        if(slave->io_next_byte == 0)
        {
            if(slave->write_reg_handler != NULL && allow_write)
            {
                slave->write_reg_handler(slave->cmd_byte);
            }       
        }
        else
        {
            if(slave->write_data_handler != NULL && allow_write)
            {
                slave->write_data_handler(slave->cmd_byte, &slave->smbus_data);
            }   
        }
    }  
    else
    if(slave->io_next_byte == 0 && !slave->is_cmd_received && !slave->is_cmd_sent)
    {
        if(slave->quick_handler != NULL)
        {
            slave->quick_handler(slave->is_quick_on);
        }
    }
    
    slave->is_cmd_received = false;
    slave->is_cmd_sent = false;
    slave->is_quick_on = false;
    slave->is_restarted = false;
    slave->io_next_byte = 0;
    slave->cmd_byte = 0x00;
    memset(&slave->smbus_data, 0, sizeof(smbus_data_t));
}

void smbus_slave_irq_rx_full(uint bus_index)
{
    i2c_inst_t* i2c = i2c_get_instance(bus_index);
    smbus_slave_t* slave = &smbus_slaves[bus_index];

    if(slave->is_cmd_received)
    {
        slave->smbus_data.block[slave->io_next_byte] = i2c_read_byte_raw(i2c);
        slave->io_next_byte += 1;
    }
    else
    {
        slave->cmd_byte = i2c_read_byte_raw(i2c);
        slave->is_cmd_received = true;
    }
}

void smbus_slave_irq_rd_req(uint bus_index)
{
    i2c_inst_t* i2c = i2c_get_instance(bus_index);
    smbus_slave_t* slave = &smbus_slaves[bus_index];

    if(slave->is_cmd_received || slave->is_cmd_sent)
    {
        i2c_write_byte_raw(i2c, slave->smbus_data.block[slave->io_next_byte]);
        slave->io_next_byte += 1;
    }
    else
    {
        busy_wait_us(2);

        if(gpio_get(slave->sda_pin))
        {
            if(slave->read_reg_handler != NULL)
            {
                slave->cmd_byte = slave->read_reg_handler();

                if(slave->is_pec_enabled)
                {
                    uint8_t read_address = smbus_get_unshifted_address(bus_index, true);
                    uint8_t crc = 0;

                    crc = smbus_pec_single(crc, read_address);
                    crc = smbus_pec_single(crc, slave->cmd_byte);

                    slave->smbus_data.block[slave->io_next_byte] = crc;
                }
            }

            slave->is_cmd_sent = true;
        }
        else
        {
            slave->cmd_byte = 0xFF;
            slave->is_quick_on = true;
        }

        i2c_write_byte_raw(i2c, slave->cmd_byte);
    }
}

void smbus_slave_irq_handler(void)
{
    uint bus_index = __get_current_exception() - VTABLE_FIRST_IRQ - I2C0_IRQ;
    i2c_inst_t* i2c = i2c_get_instance(bus_index);
    i2c_hw_t* hw = i2c_get_hw(i2c);    
    uint32_t intr_stat = hw->intr_stat;


    if(intr_stat & I2C_IC_INTR_STAT_R_RESTART_DET_BITS)
    {
        smbus_slave_irq_restart(bus_index);
        hw->clr_restart_det;

        return;
    }

    if(intr_stat & I2C_IC_INTR_STAT_R_START_DET_BITS)
    {
        smbus_slave_irq_start(bus_index);
        hw->clr_start_det; 

        return;
    }

    if(intr_stat & I2C_IC_INTR_STAT_R_TX_ABRT_BITS)
    {
        smbus_slave_irq_tx_abrt(bus_index);        
        hw->clr_tx_abrt;

        return;
    }

    if(intr_stat & I2C_IC_INTR_STAT_R_STOP_DET_BITS)
    {
        smbus_slave_irq_stop(bus_index);
        hw->clr_stop_det;  

        return;  
    }

    if(intr_stat & I2C_IC_INTR_STAT_R_RX_FULL_BITS)
    {    
        smbus_slave_irq_rx_full(bus_index);

        return;
    }    

    if(intr_stat & I2C_IC_INTR_STAT_R_RD_REQ_BITS)
    {
        smbus_slave_irq_rd_req(bus_index);
        hw->clr_rd_req;

        return;
    }
}

void smbus_init_i2c_gpio(uint gpio)
{
    gpio_init(gpio);
    gpio_set_function(gpio, GPIO_FUNC_I2C);
    gpio_pull_up(gpio);
}

uint8_t smbus_get_unshifted_address(uint bus_index, bool readwrite_bit)
{
    i2c_inst_t* i2c = i2c_get_instance(bus_index);
    i2c_hw_t* hw = i2c_get_hw(i2c);

    uint8_t unshifted_address = ((uint8_t)(hw->sar) << 1);;

    if(readwrite_bit)
    {
        unshifted_address |= 0x1;
    }

    return unshifted_address;
}


void smbus_slave_init(
    i2c_inst_t* i2c, 
    uint8_t address, 
    uint baudrate,
    uint sda_pin,
    uint scl_pin
)
{
    assert(i2c == i2c0 || i2c == i2c1);
    assert(SMBUS_MIN_BAUD_RATE_HZ <= baudrate && baudrate <= SMBUS_MAX_BAUD_RATE_HZ);
    
    i2c_hw_t* hw = i2c_get_hw(i2c);

    uint i2c_index = i2c_hw_index(i2c);
    uint intr_num = I2C0_IRQ + i2c_index;
    smbus_slave_t* slave = &smbus_slaves[i2c_index];

    memset(slave, 0, sizeof(smbus_slave_t));

    smbus_init_i2c_gpio(sda_pin);
    smbus_init_i2c_gpio(scl_pin);

    i2c_init(i2c, baudrate);
    i2c_set_slave_mode(i2c, true, address);
    
    hw->intr_mask = I2C_IC_INTR_MASK_M_START_DET_BITS
                  | I2C_IC_INTR_MASK_M_RESTART_DET_BITS
                  | I2C_IC_INTR_MASK_M_TX_ABRT_BITS
                  | I2C_IC_INTR_MASK_M_STOP_DET_BITS
                  | I2C_IC_INTR_MASK_M_RX_FULL_BITS
                  | I2C_IC_INTR_MASK_M_RD_REQ_BITS;

    irq_set_exclusive_handler(intr_num, smbus_slave_irq_handler);
    irq_set_enabled(intr_num, true);

    slave->sda_pin = sda_pin;
    slave->scl_pin = scl_pin;
}

void smbus_slave_deinit(
    i2c_inst_t* i2c
)
{
    assert(i2c == i2c0 || i2c == i2c1);

    i2c_hw_t *hw = i2c_get_hw(i2c);

    uint i2c_index = i2c_hw_index(i2c);
    uint intr_num = I2C0_IRQ + i2c_index;
    smbus_slave_t* slave = &smbus_slaves[i2c_index];

    irq_set_enabled(intr_num, false);
    irq_remove_handler(intr_num, smbus_slave_irq_handler);
    hw->intr_mask = I2C_IC_INTR_MASK_RESET;

    i2c_set_slave_mode(i2c, false, 0);
    
    gpio_deinit(slave->sda_pin);
    gpio_deinit(slave->scl_pin);
    
    memset(slave, 0, sizeof(smbus_slave_t));
}


void smbus_set_quick_handler(i2c_inst_t* i2c, quick_handler_t quick_handler)
{
    uint i2c_index = i2c_hw_index(i2c);
    smbus_slave_t* slave = &smbus_slaves[i2c_index];

    slave->quick_handler = quick_handler;
}

void smbus_set_write_reg_handler(i2c_inst_t* i2c, write_reg_handler_t handler)
{
    uint i2c_index = i2c_hw_index(i2c);
    smbus_slave_t* slave = &smbus_slaves[i2c_index];

    slave->write_reg_handler = handler;
}

void smbus_set_write_data_handler(i2c_inst_t* i2c, write_data_handler_t handler)
{
    uint i2c_index = i2c_hw_index(i2c);
    smbus_slave_t* slave = &smbus_slaves[i2c_index];

    slave->write_data_handler = handler;
}

void smbus_set_read_reg_handler(i2c_inst_t* i2c, read_reg_handler_t handler)
{
    uint i2c_index = i2c_hw_index(i2c);
    smbus_slave_t* slave = &smbus_slaves[i2c_index];

    slave->read_reg_handler = handler;
}

void smbus_set_read_data_handler(i2c_inst_t* i2c, read_data_handler_t handler)
{
    uint i2c_index = i2c_hw_index(i2c);
    smbus_slave_t* slave = &smbus_slaves[i2c_index];

    slave->read_data_handler = handler;
}

void smbus_set_proc_call_handler(i2c_inst_t* i2c, proc_call_handler_t handler)
{
    uint i2c_index = i2c_hw_index(i2c);
    smbus_slave_t* slave = &smbus_slaves[i2c_index];

    slave->proc_call_handler = handler;
}

void smbus_reset_handler(i2c_inst_t* i2c, smbus_slave_event_t slave_event)
{
    uint i2c_index = i2c_hw_index(i2c);
    smbus_slave_t* slave = &smbus_slaves[i2c_index];
    
    switch (slave_event)
    {
        case SMBUS_SLAVE_QUICK:
            slave->quick_handler = NULL;
            break;
        case SMBUS_SLAVE_WRITE_REG:
            slave->write_reg_handler = NULL;
            break;
        case SMBUS_SLAVE_WRITE_DATA:
            slave->write_data_handler = NULL;
            break;
        case SMBUS_SLAVE_READ_REG:
            slave->read_reg_handler = NULL;
            break;
        case SMBUS_SLAVE_READ_DATA:
            slave->read_data_handler = NULL;
            break;
        case SMBUS_SLAVE_PROC_CALL:
            slave->proc_call_handler = NULL;
            break;
    }
}

void smbus_set_pec(i2c_inst_t* i2c, bool is_enabled)
{
    uint i2c_index = i2c_hw_index(i2c);
    smbus_slave_t* slave = &smbus_slaves[i2c_index];

    slave->is_pec_enabled = is_enabled;
}

bool smbus_get_pec(i2c_inst_t* i2c)
{
    uint i2c_index = i2c_hw_index(i2c);
    smbus_slave_t* slave = &smbus_slaves[i2c_index];

    return slave->is_pec_enabled;
}

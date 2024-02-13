#include <smbus_slave.h>
#include <hardware/irq.h>
#include <hardware/gpio.h> 
#include <string.h>


#define SMBUS_MIN_BAUD_RATE_HZ _u(10000)
#define SMBUS_MAX_BAUD_RATE_HZ _u(100000)

#define SMBUS_CALLBACK(callback, ...)   \
    if(callback != NULL)                \
    {                                   \
        callback(__VA_ARGS__);          \
    }


typedef struct smbus_slave_t
{
    quick_handler_t quick_handler;
    byte_handler_t read_byte_handler;
    byte_handler_t write_byte_handler;
    data_handler_t read_data_handler;
    data_handler_t write_data_handler;
    proc_call_handler_t proc_call_handler;

    uint scl_pin;
    uint sda_pin;

    bool is_cmd_read;
    bool is_restarted;
    bool is_quick_on;

    uint8_t cmd_byte;
    smbus_data_t smbus_data;
    uint8_t io_next_byte;
}
smbus_slave_t;

static smbus_slave_t smbus_slaves[2];

static void __isr __not_in_flash_func(smbus_slave_irq_restart)(smbus_slave_t* slave);
static void __isr __not_in_flash_func(smbus_slave_irq_start)(smbus_slave_t* slave);
static void __isr __not_in_flash_func(smbus_slave_irq_stop)(smbus_slave_t* slave);
static void __isr __not_in_flash_func(smbus_slave_irq_tx_abrt)(smbus_slave_t* slave, uint32_t abrt_source);
static void __isr __not_in_flash_func(smbus_slave_irq_rx_full)(smbus_slave_t* slave, i2c_inst_t* i2c);
static void __isr __not_in_flash_func(smbus_slave_irq_rd_req)(smbus_slave_t* slave, i2c_inst_t* i2c);
static void __isr __not_in_flash_func(smbus_slave_irq_handler)(void);
static void smbus_init_i2c_gpio(uint gpio);

void smbus_slave_irq_restart(smbus_slave_t* slave)
{
    slave->is_restarted = true;
}

void smbus_slave_irq_start(smbus_slave_t* slave)
{
    if(slave->is_restarted)
    {
        if(slave->io_next_byte == 0)
        {
            SMBUS_CALLBACK(slave->read_data_handler, slave->cmd_byte, &slave->smbus_data);
        }
        else
        if(slave->io_next_byte == 2)
        {
            uint16_t request = slave->smbus_data.word;
            uint16_t response = 0;

            SMBUS_CALLBACK(slave->proc_call_handler, slave->cmd_byte, &request, &response);

            slave->smbus_data.word = response;
            slave->io_next_byte = 0;
        }
    }
}

void smbus_slave_irq_tx_abrt(smbus_slave_t* slave, uint32_t abrt_source)
{}

void smbus_slave_irq_stop(smbus_slave_t* slave)
{
    if(!slave->is_cmd_read)
    {
        SMBUS_CALLBACK(slave->quick_handler, slave->is_quick_on);
    }
    else
    {
        if(slave->io_next_byte == 0)
        {
            SMBUS_CALLBACK(slave->write_byte_handler, &slave->cmd_byte);
        }
        else
        {
            SMBUS_CALLBACK(slave->write_data_handler, slave->cmd_byte, &slave->smbus_data);
        }
    }  

    slave->is_cmd_read = false;
    slave->is_restarted = false;
    slave->is_quick_on = false;
    slave->io_next_byte = 0;
    slave->cmd_byte = 0x00;
    memset(&slave->smbus_data, 0, sizeof(smbus_data_t));
}

void smbus_slave_irq_rx_full(smbus_slave_t* slave, i2c_inst_t* i2c)
{
    if(slave->is_cmd_read)
    {
        slave->smbus_data.block[slave->io_next_byte] = i2c_read_byte_raw(i2c);
        slave->io_next_byte += 1;
    }
    else
    {
        slave->cmd_byte = i2c_read_byte_raw(i2c);
        slave->is_cmd_read = true;
    }
}

void smbus_slave_irq_rd_req(smbus_slave_t* slave, i2c_inst_t* i2c)
{
    if(slave->is_cmd_read)
    {
        i2c_write_byte_raw(i2c, slave->smbus_data.block[slave->io_next_byte]);
        slave->io_next_byte += 1;
    }
    else
    {
        if(gpio_get(slave->sda_pin))
        {
            SMBUS_CALLBACK(slave->read_byte_handler, &slave->cmd_byte);
            slave->is_cmd_read = true;
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
    uint i2c_index = __get_current_exception() - VTABLE_FIRST_IRQ - I2C0_IRQ;
    i2c_inst_t* i2c = i2c_get_instance(i2c_index);
    i2c_hw_t* hw = i2c_get_hw(i2c);
    smbus_slave_t* slave = &smbus_slaves[i2c_index];
    
    uint32_t intr_stat = hw->intr_stat;


    if(intr_stat & I2C_IC_INTR_STAT_R_RESTART_DET_BITS)
    {
        smbus_slave_irq_restart(slave);
        hw->clr_restart_det;

        return;
    }

    if(intr_stat & I2C_IC_INTR_STAT_R_START_DET_BITS)
    {
        smbus_slave_irq_start(slave);
        hw->clr_start_det; 

        return;
    }

    if(intr_stat & I2C_IC_INTR_STAT_R_TX_ABRT_BITS)
    {
        smbus_slave_irq_tx_abrt(slave, hw->tx_abrt_source);        
        hw->clr_tx_abrt;

        return;
    }

    if(intr_stat & I2C_IC_INTR_STAT_R_STOP_DET_BITS)
    {
        smbus_slave_irq_stop(slave);
        hw->clr_stop_det;  

        return;  
    }

    if(intr_stat & I2C_IC_INTR_STAT_R_RX_FULL_BITS)
    {    
        smbus_slave_irq_rx_full(slave, i2c);

        return;
    }    

    if(intr_stat & I2C_IC_INTR_STAT_R_RD_REQ_BITS)
    {
        smbus_slave_irq_rd_req(slave, i2c);
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

void smbus_set_read_byte_handler(i2c_inst_t* i2c, byte_handler_t byte_handler)
{
    uint i2c_index = i2c_hw_index(i2c);
    smbus_slave_t* slave = &smbus_slaves[i2c_index];

    slave->read_byte_handler = byte_handler;
}

void smbus_set_write_byte_handler(i2c_inst_t* i2c, byte_handler_t byte_handler)
{
    uint i2c_index = i2c_hw_index(i2c);
    smbus_slave_t* slave = &smbus_slaves[i2c_index];

    slave->write_byte_handler = byte_handler;
}

void smbus_set_read_data_handler(i2c_inst_t* i2c, data_handler_t data_handler)
{
    uint i2c_index = i2c_hw_index(i2c);
    smbus_slave_t* slave = &smbus_slaves[i2c_index];

    slave->read_data_handler = data_handler;
}

void smbus_set_write_data_handler(i2c_inst_t* i2c, data_handler_t data_handler)
{
    uint i2c_index = i2c_hw_index(i2c);
    smbus_slave_t* slave = &smbus_slaves[i2c_index];

    slave->write_data_handler = data_handler;
}

void smbus_set_proc_call_handler(i2c_inst_t* i2c, proc_call_handler_t proc_call_handler)
{
    uint i2c_index = i2c_hw_index(i2c);
    smbus_slave_t* slave = &smbus_slaves[i2c_index];

    slave->proc_call_handler = proc_call_handler;
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
        case SMBUS_SLAVE_READ_BYTE:
            slave->read_byte_handler = NULL;
            break;
        case SMBUS_SLAVE_WRITE_BYTE:
            slave->write_byte_handler = NULL;
            break;
        case SMBUS_SLAVE_READ_DATA:
            slave->read_data_handler = NULL;
            break;
        case SMBUS_SLAVE_WRITE_DATA:
            slave->write_data_handler = NULL;
            break;
        case SMBUS_SLAVE_PROC_CALL:
            slave->proc_call_handler = NULL;
            break;
    }
}
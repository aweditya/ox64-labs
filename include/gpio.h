#pragma once

#include "ox64.h"

typedef struct {
    uint32_t input_enable   : 1;
    uint32_t smt_control    : 1;
    uint32_t drv_control    : 2;
    uint32_t pu_control     : 1;
    uint32_t pd_control     : 1;
    uint32_t output_enable  : 1;
    uint32_t rsvd1          : 1;
    uint32_t func_sel       : 5;
    uint32_t rsvd2          : 3;
    uint32_t int_mode_set   : 4;
    uint32_t int_clr        : 1;
    uint32_t int_stat       : 1;
    uint32_t int_mask       : 1;
    uint32_t rsvd3          : 1;
    uint32_t output_val     : 1;
    uint32_t set            : 1;
    uint32_t clr            : 1;
    uint32_t rsvd4          : 1;
    uint32_t input_val      : 1;
    uint32_t rsvd5          : 1;
    uint32_t mode           : 2;
} __attribute__((packed)) gpio_cfg_t;

// gpio func sel enum
typedef enum {
    GPIO_FUNC_SPI       = 1,
    GPIO_FUNC_FLASH     = 2,
    GPIO_FUNC_I2S       = 3,
    GPIO_FUNC_PDM       = 4,
    GPIO_FUNC_I2C0      = 5,
    GPIO_FUNC_I2C1      = 6,
    GPIO_FUNC_UART0     = 7,
    GPIO_FUNC_EMAC      = 8,
    GPIO_FUNC_CAM       = 9,
    GPIO_FUNC_ANALOG    = 10, 
    GPIO_FUNC_GPIO      = 11, 
    GPIO_FUNC_PWM0      = 16, 
    GPIO_FUNC_PWM1      = 17, 
    GPIO_FUNC_MM_SPI    = 18, 
    GPIO_FUNC_MM_I2C0   = 19, 
    GPIO_FUNC_MM_I2C1   = 20, 
    GPIO_FUNC_MM_UART   = 21, 
    GPIO_FUNC_DBI_B     = 22, 
    GPIO_FUNC_DBI_C     = 23, 
    GPIO_FUNC_DPI       = 24, 
    GPIO_FUNC_M0_JTAG   = 26, 
    GPIO_FUNC_D0_JTAG   = 27, 
} gpio_func_t;

// gpio mode enum
typedef enum {
    // gpio op by reg_gpio_x_o
    op_val_mode     = 0b00,

    // gpio op set by reg_gpio_x_set
    // gpio op clr by reg_gpio_x_clr
    set_clr_mode    = 0b01,

    // swgpio src from gpio dma
    // gpio op by gpio_dma_o
    swgpio_mode1    = 0b10,

    // swgpio src from gpio dma
    // gpio op by gpio_dma_set/clr
    swgpio_mode2    = 0b11,
} gpio_op_mode_t;

// gpio intr mode enum
enum {
    sync_fall_edge  = 0b0000,
    sync_rise_edge  = 0b0001,
    sync_lo_level   = 0b0010,
    sync_hi_level   = 0b0011,

    // [1:0] can be anything
    // i chose 00
    sync_both_edge  = 0b0100,

    async_fall_edge = 0b1000,
    async_rise_edge = 0b1001,
    async_lo_level  = 0b1010,
    async_hi_level  = 0b1011,
};

static volatile gpio_cfg_t *const GPIO_CFG0 = (volatile gpio_cfg_t *)0x200008c4;

void gpio_set_function(unsigned pin, gpio_func_t function);

void gpio_set_input(unsigned pin);
void gpio_set_output(unsigned pin);

void gpio_write(unsigned pin, unsigned val);

int gpio_read(unsigned pin);

void gpio_set_on(unsigned pin);
void gpio_set_off(unsigned pin);

void gpio_set_pullup(unsigned pin);
void gpio_set_pulldown(unsigned pin);
void gpio_pud_off(unsigned pin);

int gpio_get_pud(unsigned pin);


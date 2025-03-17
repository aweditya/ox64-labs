#pragma once
#include <stdint.h>
#include <stdbool.h>

#include "gpio.h"

#define GPIO_TX 14
#define GPIO_RX 15

#define UART_CLK 40000000UL

// we only focus on uart0 which is needed to communicate
// with our pc
static volatile uint32_t *const UART0_TX_CFG   = (volatile uint32_t *)0x2000a000;
static volatile uint32_t *const UART0_RX_CFG   = (volatile uint32_t *)0x2000a004;

static volatile uint32_t *const UART0_BAUD     = (volatile uint32_t *)0x2000a008;

// disabling uart interrupts for now. probably a good
// idea to have a separate structure for this
static volatile uint32_t *const UART0_INTR      = (volatile uint32_t *)0x2000a02c;

static volatile uint32_t *const UART_FIFO_CFG0  = (volatile uint32_t *)0x2000a080;
static volatile uint32_t *const UART_FIFO_CFG1  = (volatile uint32_t *)0x2000a084;

static volatile uint32_t *const UART_FIFO_WDATA = (volatile uint32_t *)0x2000a088;
static volatile uint32_t *const UART_FIFO_RDATA = (volatile uint32_t *)0x2000a08c;


void uart_init(unsigned baud) {
    // setup gpio pins as uart
    gpio_set_function(GPIO_TX, GPIO_FUNC_UART0);
    gpio_set_function(GPIO_RX, GPIO_FUNC_UART0);

    // disable all uart interrupts
    put32(UART0_INTR, 0);

    // set the baud rate
    // same for both rx and tx
    // for some reason, you need to divide by 2
    uint32_t bit_prd = (2 * UART_CLK / baud) - 1;
    put32(UART0_BAUD, bit_prd << 16 | bit_prd);

    // clear rx and tx fifo
    put32(UART_FIFO_CFG0, 1<<2|1<<3);

    // enable tx 
    uint32_t utxcfg = 0;
    utxcfg |= 1 << 0;           // tx enable
    utxcfg |= 1 << 2;           // free run enable
    utxcfg |= 7 << 8;           // bitcnt (7 data)
    utxcfg |= 2 << 11;          // stop bitcnt (units of 0.5)
    put32(UART0_TX_CFG, utxcfg);

    // enable rx
    uint32_t urxcfg = 0;
    urxcfg |= 1 << 0;           // rx enable
    urxcfg |= 7 << 8;           // bitcnt (7 data)
    put32(UART0_RX_CFG, urxcfg);
}

bool uart_can_getc(void) {
    uint32_t fifo_cfg1 = get32(UART_FIFO_CFG1);
    uint32_t rx_fifo_cnt = (fifo_cfg1 >> 8) & 0b111111;
    return rx_fifo_cnt > 0;
}

uint8_t uart_getc(void) {
    while (!uart_can_getc())
        ;

    uint32_t rdata = get32(UART_FIFO_RDATA);
    return (rdata & 0xff);
}

bool uart_can_putc(void) {
    uint32_t fifo_cfg1 = get32(UART_FIFO_CFG1);
    uint32_t tx_fifo_cnt = fifo_cfg1 & 0b111111;
    return tx_fifo_cnt > 0;
}

void uart_putc(uint8_t c) {
    while (!uart_can_putc());
    put32(UART_FIFO_WDATA, c);
}

void uart_puts(const char *c) {
    while (*c) {
        uart_putc(*c++);
    }
}

/*
 * UART documentation begins on page. 402 of the Bouffalo labs BL808 documentation
 *
 * Page numbers refer to this documentation.
 */

#include "uart.h"

void uart_init(unsigned baud) {
    // setup gpio pins as uart
    gpio_set_function(GPIO_TX, GPIO_FUNC_UART0);
    gpio_set_function(GPIO_RX, GPIO_FUNC_UART0);

    // disable all uart interrupts
    put32(UART0_INTR, 0);

    // set the baud rate
    // Pg. 405 Bouffalo
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
    // bits 13:8 of UART_FIFO_CFG1
    // store RX FIFO available count
    uint32_t fifo_cfg1 = get32(UART_FIFO_CFG1);
    uint32_t rx_fifo_cnt = (fifo_cfg1 >> 8) & 0b111111;
    return rx_fifo_cnt > 0;
}

uint8_t uart_getc(void) {
    while (!uart_can_getc())
        ;

    // Page 429: UART stores next byte of data in bits 7:0
    // of UART_FIFO_RDATA
    uint32_t rdata = get32(UART_FIFO_RDATA);
    return (rdata & 0xff);
}

bool uart_can_putc(void) {
    // Page 428
    // Bits 5:0 of UART_FIFO_CFG1 store the available count of the
    // tx fifo
    uint32_t fifo_cfg1 = get32(UART_FIFO_CFG1);
    uint32_t tx_fifo_cnt = fifo_cfg1 & 0b111111;
    return tx_fifo_cnt > 0;
}

void uart_putc(uint8_t c) {
    while (!uart_can_putc());
    // Page 428
    // First byte of UART_FIFO_WDATA is where we write 
    // data to 
    put32(UART_FIFO_WDATA, c);
}

void uart_puts(const char *c) {
    // Simple wrapper. Just putc for each character in the string
    while (*c) {
        uart_putc(*c++);
    }
}

/*
 * UART documentation begins on page. 402 of the Bouffalo labs BL808 documentation
 *
 * Page numbers refer to this documentation.
 */

#pragma once
#include <stdint.h>
#include <stdbool.h>

#include "gpio.h"

// Pins for UART0 on the OX64
#define GPIO_TX 14
#define GPIO_RX 15

#define UART_CLK 40000000UL

// we only focus on uart0 which is needed to communicate
// with our pc
// Register Description on Page 415
volatile uint32_t *const UART0_TX_CFG   = (volatile uint32_t *)0x2000a000;
// Register Description on Page 416
volatile uint32_t *const UART0_RX_CFG   = (volatile uint32_t *)0x2000a004;

// Register Description on Page 417
volatile uint32_t *const UART0_BAUD     = (volatile uint32_t *)0x2000a008;

// disabling uart interrupts for now. probably a good
// idea to have a separate structure for this
// Register Description on Page 422
volatile uint32_t *const UART0_INTR      = (volatile uint32_t *)0x2000a02c;

// Register description on Page 427.
volatile uint32_t *const UART_FIFO_CFG0  = (volatile uint32_t *)0x2000a080;
// Register description on Page 427-428
volatile uint32_t *const UART_FIFO_CFG1  = (volatile uint32_t *)0x2000a084;


// Register description on Page 428
volatile uint32_t *const UART_FIFO_WDATA = (volatile uint32_t *)0x2000a088;
// Register description on Page 428-429
volatile uint32_t *const UART_FIFO_RDATA = (volatile uint32_t *)0x2000a08c;

/*
 * Initialize the uart with the given baud rate
 *
 * Initializes with 7 data bits, 1 stop bit
 *
 * Puts the UART TX in FreeRun mode, which is described on page. 404
 *
 * Enables RX with 7 data bits
 */
void uart_init(unsigned baud);

// Check if the uart can get a character from the rx
bool uart_can_getc(void);

// Get character from the uart receiver
uint8_t uart_getc(void);

// Check if the uart can put a character to the tx
bool uart_can_putc(void);

// Put character to the uart tx
void uart_putc(uint8_t c);

// Put string to the uart tx
void uart_puts(const char *c);
# Bare-metal Programming with the Ox64 Board

## GPIO: BL808 pages 54 - 168

### pg 54-55 have the function list for the gpio function select bits
right now the only important one is 11 for GPIO

### pg 64 - gpio_cfg_0
Address is 0x200008c4, further pins increment addr by 4

Important bits:
- 0 - input enable
- 6 - output enable
- 8:12 - function select
- 25 - set
- 26 - clear
- 28 - value (use when getting value of input pin, confusingly named reg_gpio_0_i with no description)
- 30:31 - mode

### configure pin as output
- set output enable bit
- set function select to GPIO
- set mode to set/clear (0b01)
set/clr mode lets us set the pin's output val by modifying the set and clear bits of the pin's config register

### set output pin to high
- set clear bit to 0
- set set bit to 1

### set output pin to low
- set set bit to 0
- set clear bit to 1

Alternative method: set mode to output value mode and write to bit 24 (output) to set value

### configure pin as input
- set input enable bit
- set function select to GPIO
- to get value, can now just read value bit

## UART: BL808 pages 402 - 429
important registers:
- utx_config, pg 415-416
- urx_config, pg 416-417
- uart_bit_prd, pg 417 (use to set baud rate)
- uart_int_en, pg 422-423
- uart_fifo_config_0, pg 427
- uart_fifo_config_1, pg 427-428
- uart_fifo_wdata, pg 428
- uart_fifo_rdata, pg 428-429

### to init:
- set tx and rx pins to function 7 (UART0, as given by function list on pg 55)
- disable uart interrupts
- set baudrate for both rx and tx to same thing
- clear rx and tx fifos by writing to fifo_config_0 clear bits (2 and 3)

### to enable tx, write to utx_config
- set bit 0 (enable tx)
- set bit 2 (enable free run mode)
- set bits 8:10 to 7 (7 data bits)
- set bits 11:12 to 2 (1 stop bit, register uses units of 0.5)
we use free run mode (described on pg 404) so the transmitter continues to send while there is data in the tx fifo

### to enable rx, write to urx_config
- set bit 0 (enable rx)
- set bits 8:10 to 7 (7 data bits)

### to send a char
- read fifo_config_1 bits 0:5 to get the number of free bits in the tx fifo, if not 0 can add more
- then just put char in uart_fifo_wdata

## to receive a char
- read fifo_config_1 bits 8:13 to get the number of available bits in the rx fifo, if greater than 0 can read
- then just read from uart_fifo_rdata bits 0:7
- read from uart_fifo_rdata

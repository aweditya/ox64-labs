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

### to receive a char
- read fifo_config_1 bits 8:13 to get the number of available bits in the rx fifo, if greater than 0 can read
- then just read from uart_fifo_rdata bits 0:7
- read from uart_fifo_rdata

## Interrupts

Control registers
- MSTATUS control register, c906 pg 618-621
  - bit 7, MIE, used for global interrupt enable/disable
- MIE (machine interrupt enable) control register, c906 pg 623-624
  - also called MIE, yes, but its own register not the MSTATUS bit
  - bit 3, MSIE, software interrupt enable
  - bit 7, MTIW, timer interrupt enable
  - bit 11, MEIE, external interrupt enable
- MTVEC (vector base addr) control register, c906 pg 624-625
  - put address of interrupt handler function here 
  - lower two bits are used to set mode, we use 00 so handler catches both exceptions and interrupts
- MEPC, c906 pg 625
  - check to get PC to be returned when CPU exits exception (16 bit aligned)
- MCAUSE, c906 pg 626
  - check to get vector number of events that trigger exceptions
- MTVAL, c906 pg 19-20
  - table on 20 describes what value it holds depending on type of exception
- MIP (interrupt pending) control register, c906 pg 626-628
  - check to see if interrupts are pending
- MSIP0 (software interrupt pending), c906 pg 84

## Software Interrupts

to enable software interrupts, make sure you enable global interrupts in MSTATUS, and enable software interrupts in the MIE

to generate software interrupt, write 1 to MSIP0

to clear software interrupt, write 0 to MSIP0

## CLINT (core local interrupts)

CLINT registers

Have to offset all memory access by 0xe0000000, where did we find this? had to look at the boufallo sdk :(

- MTIMECMPL0 (timer compare value lower 32 bits), c906 pg 84
- MTIMECMPH0 (timer compare value upper 32 bits), c906 pg 84

to enable timer interrupts, make sure you enable global interrupts in MSTATUS, and enable timer interrupts in the MIE

then it's as simple as writing to MTIMECMPL0 and MTIMECMPH0 the timer value you want to fault upon reaching

however, no way to reset the counter. Instead, in the handler, can increment MTIMECMP by some value. Since it's a 64 bit value, it probably won't wrap around anytime soon! 

## PLIC (not working...)
c906 pg 88 - 95

all PLIC addrs must be offset by the value in the mapbaddr register (c906 pg 637). Is this mentioned anywhere in the PLIC chapter? No, of course not

as best as we can tell, setup process for plic in general is
- clear outstanding interrupts by writing back mclaim register value to itself
- set interrupt threshold to 0 to accept all interrupts
- set interrupt priorities for interrupts you want to 1
- enable external interrupts in MIE
- enable global interrupts in MSTATUS

to set up timer interrupts
- clear timer interrupts by writing to clear register
- set mode to preload in control register
- set preload value to something (counter will reset to this after hitting match val)
- set preload match val by writing to preload ctrl register (which match register to reset to preload val after matching)
- set match value register(s) with value you want clock to fault on
- set clock source
- set clock freq divider (divides clock frequency by register val + 1 (so 0 is no division, 1 is divide by 2, etc))
- enable timer interrupts for one of the match values

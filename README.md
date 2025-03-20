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
- 28 - value (confusingly named reg_gpio_0_i with no description)
- 30:31 - mode

### configure pin as output
- set output enable bit
- set function select to GPIO
- set mode to set/clr (0b01)

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

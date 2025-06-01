#define LOG_LEVEL 3
#include "lib.h"

// Function to read the mvendorid CSR register
static inline uint64_t read_mvendorid(void) {
    uint64_t result;
    // csrrw x0, mvendorid, x0 is the equivalent of reading the register
    asm volatile("csrr %0, mvendorid" : "=r"(result)); 
    return result;
}

// Function to read the marchid CSR register
static inline uint64_t read_marchid(void) {
    uint64_t result;
    // csrrw x0, marchid, x0 is the equivalent of reading the register
    asm volatile("csrr %0, marchid" : "=r"(result)); 
    return result;
}

// Function to read the mimpid CSR register
static inline uint64_t read_mimpid(void) {
    uint64_t result;
    // csrrw x0, mimpid, x0 is the equivalent of reading the register
    asm volatile("csrr %0, mimpid" : "=r"(result)); 
    return result;
}

// Function to read the mhartid CSR register
static inline uint64_t read_mhartid(void) {
    uint64_t result;
    // csrrw x0, mhartid, x0 is the equivalent of reading the register
    asm volatile("csrr %0, mhartid" : "=r"(result)); 
    return result;
}

// Function to read the misa CSR register
static inline uint64_t read_misa(void) {
    uint64_t result;
    // csrrw x0, misa, x0 is the equivalent of reading the register
    asm volatile("csrr %0, misa" : "=r"(result)); 
    return result;
}

// Function to read the mstatus CSR register
static inline uint64_t read_mstatus(void) {
    uint64_t result;
    // csrrw x0, mstatus, x0 is the equivalent of reading the register
    asm volatile("csrr %0, mstatus" : "=r"(result)); 
    return result;
}


// Helper function to convert an integer to a hexadecimal string and print using putc
void itoa_hex(uint64_t num) {
    static const char hex_digits[] = "0123456789ABCDEF";

    // Print the '0x' prefix
    uart_putc(UART0, '0');
    uart_putc(UART0, 'x');

    // Print each hexadecimal digit
    for (int i = 60; i >= 0; i -= 4) {
        uart_putc(UART0, hex_digits[(num >> i) & 0xF]);
    }
}

void kmain(void) {
  uart_init(UART0, 115200);
  uart_puts(UART0, "entered hello\n");
  gpio_set_output(16);
  gpio_set_output(17);
  gpio_set_input(11);

  uint64_t mstatus = read_mstatus();
  uint64_t mvendorid = read_mvendorid();
  uint64_t marchid = read_marchid();
  uint64_t mimpid = read_mimpid();
  uint64_t mhartid = read_mhartid();
  uint64_t misa = read_misa();

  char buffer[19]; // Buffer to store the hexadecimal string, including "0x" and null terminator


  int v = 0;
  while (true) {
    itoa_hex(mstatus);
    uart_putc(UART0, '\n');
    itoa_hex(mvendorid);
    uart_putc(UART0, '\n');
    itoa_hex(marchid);
    uart_putc(UART0, '\n');
    itoa_hex(mimpid);
    uart_putc(UART0, '\n');
    itoa_hex(mhartid);
    uart_putc(UART0, '\n');
    itoa_hex(misa);
    uart_putc(UART0, '\n');

    unsigned long start = timer_read();
    gpio_write(16, v);
    gpio_write(17, v);

    int vv = gpio_read(11);
    if (vv == 0)
        v = 1;
    else
        v = 0;

    delay_ms(1000);
  }
}

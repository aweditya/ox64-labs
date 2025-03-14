#define LOG_LEVEL 3
#include "lib.h"

static volatile uint32_t *const MSIP0 = (volatile uint32_t *)0xe4000000;
static volatile uint32_t *const MTIMECMPL0 = (volatile uint32_t *)0xe4004000;
static volatile uint32_t *const MTIMECMPH0 = (volatile uint32_t *)0xe4004004;

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
    uart_putc(UART0, '\n');
}

static inline uint64_t get_mtvec(void) {
    uint64_t result;
    asm volatile("csrr %0, mtvec" : "=r"(result));
    return result;
}


static inline uint64_t get_mstatus(void) {
    uint64_t result;
    asm volatile("csrr %0, mstatus" : "=r"(result));
    return result;
}

static inline uint64_t get_mie(void) {
    uint64_t result;
    asm volatile("csrr %0, mie" : "=r"(result));
    return result;
}

static inline uint64_t get_mip(void) {
    uint64_t result;
    asm volatile("csrr %0, mip" : "=r"(result));
    return result;
}

static inline uint64_t get_mxstatus(void) {
    uint64_t result;
    asm volatile("csrr %0, mxstatus" : "=r"(result));
    return result;
}

static inline void vector_base_set(void *vec) {
    // uint64_t base = (uint64_t)vec;
    // base = base << 2;
    uint64_t *base = (uint64_t *)vec;
    asm volatile(
            "csrw mtvec, %0"
            :
            : "r" (base));
}

__attribute__((aligned(4))) void handler(void) {
    uart_puts(UART0, "inside handler\n");
    uint64_t mepc;
    asm volatile("csrr %0, mepc" : "=r"(mepc));

    uint64_t mcause;
    asm volatile("csrr %0, mcause" : "=r"(mcause));

    uint64_t mtval;
    asm volatile("csrr %0, mtval" : "=r"(mtval));

    itoa_hex(mepc);
    uart_putc(UART0, '\n');
    itoa_hex(mcause);
    uart_putc(UART0, '\n');
    itoa_hex(mtval);
    uart_putc(UART0, '\n');
}

void disable_interrupts(void) {
    asm volatile("csrci mstatus, 0x8");
    return;
}

void enable_interrupts(void) {
    asm volatile("csrsi mstatus, 0x8");
    return;
}

void kmain(void) {
  uart_init(UART0, 115200);

  disable_interrupts();
  vector_base_set((uint64_t *)handler);

  enable_interrupts();

  while(1) {
      asm volatile("wfi");
  }
}

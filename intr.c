#define LOG_LEVEL 3
#include "lib.h"

enum {   
    MSIP0       = 0x4000000,

    MTIMECMPL0  = 0x4004000,
    MTIMECMPH0  = 0x4004004,
};

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

void handler(void) {
    uart_puts(UART0, "inside handler\n");
    uint64_t mepc;
    asm volatile("csrr %0, mepc" : "=r"(mepc));

    uint64_t mcause;
    asm volatile("csrr %0, mcause" : "=r"(mcause));

    uint64_t mtval;
    asm volatile("csrr %0, mtval" : "=r"(mtval));

    itoa_hex(mepc);
    itoa_hex(mcause);
    itoa_hex(mtval);
}

void disable_interrupts(void) {
    asm volatile("csrci mstatus, 0x8");
    return;
}

void enable_interrupts(void) {
    asm volatile("csrsi mstatus, 0x8");
    return;
}

void disable_timer_interrupts(void) {
    asm volatile("li t0, 0x80");
    asm volatile("csrrc t1, mie, t0");
    return;
}

void enable_timer_interrupts(void) {
    asm volatile("li t0, 0x80");
    asm volatile("csrrs t1, mie, t0");
    return;
}

void my_timer_init(uint32_t mtime_cmp_lo, uint32_t mtime_cmp_hi) {
    // start by disabling timer interrupts
    disable_timer_interrupts();

    PUT32(MTIMECMPL0, mtime_cmp_lo);
    PUT32(MTIMECMPH0, mtime_cmp_hi);

    // enable at the very end
    enable_timer_interrupts();
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

static inline void vector_base_set(void *vec) {
    uint64_t *base = (uint64_t *)vec;
    asm volatile(
            "csrw mtvec, %0"
            :
            : "r" (base));
}

void kmain(void) {
  uart_init(UART0, 115200);
//  while (true) {
//    char c = uart_getc(UART0);
//    uart_putc(UART0, c);
//  }
  disable_interrupts();
  uart_puts(UART0, "disabled interrupt\n");
  vector_base_set((uint64_t *)handler);

  my_timer_init(5000, 0);
  enable_interrupts();
  uart_puts(UART0, "enabled interrupt\n");

  while(1) {
      uint64_t mtvec = get_mtvec();
      uint64_t mstatus = get_mstatus();
      uint64_t mie = get_mie();
      itoa_hex(mtvec);
      uart_putc(UART0, '\n');
      itoa_hex(mstatus);
      uart_putc(UART0, '\n');
      itoa_hex(mie);
      uart_putc(UART0, '\n');
      delay_ms(1000);
  }
}

#include clint.h

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

// void handler(void) {
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

void disable_sw_interrupts(void) {
    asm volatile("csrci mie, 0x8");
}

void enable_sw_interrupts(void) {
    asm volatile("csrsi mie, 0x8");
}

void gen_sw_interrupts(void) {
    put32(MSIP0, 0x1);
}

void my_timer_init(uint32_t mtime_cmp_lo, uint32_t mtime_cmp_hi) {
    // start by disabling timer interrupts
    disable_timer_interrupts();

    put32(MTIMECMPL0, mtime_cmp_lo);
    put32(MTIMECMPH0, mtime_cmp_hi);

    // enable at the very end
    enable_timer_interrupts();
}

inline uint64_t get_mtvec(void) {
    uint64_t result;
    asm volatile("csrr %0, mtvec" : "=r"(result));
    return result;
}


inline uint64_t get_mstatus(void) {
    uint64_t result;
    asm volatile("csrr %0, mstatus" : "=r"(result));
    return result;
}

inline uint64_t get_mie(void) {
    uint64_t result;
    asm volatile("csrr %0, mie" : "=r"(result));
    return result;
}

inline uint64_t get_satp(void) {
    uint64_t result;
    asm volatile("csrr %0, satp" : "=r"(result));
    return result;
}

inline uint64_t get_mip(void) {
    uint64_t result;
    asm volatile("csrr %0, mip" : "=r"(result));
    return result;
}

inline uint64_t get_mxstatus(void) {
    uint64_t result;
    asm volatile("csrr %0, mxstatus" : "=r"(result));
    return result;
}

inline void vector_base_set(void *vec) {
    // uint64_t base = (uint64_t)vec;
    // base = base << 2;
    uint64_t *base = (uint64_t *)vec;
    asm volatile(
            "csrw mtvec, %0"
            :
            : "r" (base));
}

void kmain(void) {
  uart_init(UART0, 115200);

  disable_interrupts();
  disable_timer_interrupts();

  // disable_sw_interrupts();

  my_timer_init(0x5000, 0x0);
  vector_base_set((uint64_t *)handler);

  // enable_sw_interrupts();

  enable_timer_interrupts();
  enable_interrupts();
  uart_puts(UART0, "thx man!\n");
  // gen_sw_interrupts();
  // uint32_t x = get32(MSIP0);

  uart_puts(UART0, "i got you!\n");

  while(1) {
      // itoa_hex(x);
      // delay_ms(1000);
      asm volatile("wfi");
  }
}

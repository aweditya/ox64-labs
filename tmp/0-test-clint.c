#include "clint.h"

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



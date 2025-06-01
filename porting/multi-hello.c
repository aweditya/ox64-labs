#define LOG_LEVEL 3
#include "lib.h"

void kmain(void) {
  uart_init(UART0, 115200);

  while(1) {
      uint8_t c = uart_getc(UART0);
      uart_putc(UART0, c);
      delay_ms(1000);
  }
}

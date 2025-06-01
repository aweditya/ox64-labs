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
    uart_putc(UART0, '\r');
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

__attribute__((aligned(4))) void handler(void) {
    uart_puts(UART0, "inside handler\r\n");
    uint64_t mepc;
    asm volatile("csrr %0, mepc" : "=r"(mepc));

    uint64_t mcause;
    asm volatile("csrr %0, mcause" : "=r"(mcause));

    uint64_t mtval;
    asm volatile("csrr %0, mtval" : "=r"(mtval));

    uint32_t mtime_cmp_lo = get32(MTIMECMPL0);
    uint32_t mtime_cmp_hi = get32(MTIMECMPH0);
    uart_puts(UART0, "MTIMECMPL0: ");
    itoa_hex(mtime_cmp_lo);
    uart_puts(UART0, "MTIMECMPH0: ");
    itoa_hex(mtime_cmp_hi);

    mtime_cmp_lo += 0xffffff;
    put32(MTIMECMPL0, mtime_cmp_lo);
    put32(MTIMECMPH0, mtime_cmp_hi);

    uart_puts(UART0, "MEPC: ");
    itoa_hex(mepc);
    uart_puts(UART0, "MCAUSE: ");
    itoa_hex(mcause);
    uart_puts(UART0, "MTVAL: ");
    itoa_hex(mtval);

    enable_timer_interrupts();
    enable_interrupts();
}

void my_timer_init(uint32_t mtime_cmp_lo, uint32_t mtime_cmp_hi) {
    // start by disabling timer interrupts
    disable_timer_interrupts();

    put32(MTIMECMPL0, mtime_cmp_lo);
    put32(MTIMECMPH0, mtime_cmp_hi);

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

static inline uint64_t get_satp(void) {
    uint64_t result;
    asm volatile("csrr %0, satp" : "=r"(result));
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

void kmain(void) {
  uart_init(UART0, 115200);

  disable_interrupts();
  disable_timer_interrupts();

  my_timer_init(0xffffff, 0x0);
  vector_base_set((uint64_t *)handler);

  enable_timer_interrupts();
  enable_interrupts();

  while(1) {
      uart_puts(UART0, "hi\r\n");
      delay_ms(1000);
  }
}

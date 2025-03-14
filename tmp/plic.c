#define LOG_LEVEL 3
#include "lib.h"

static volatile uint32_t *const PLIC_H0_MIE_BASE = (volatile uint32_t *)0xe0002000;
static volatile uint32_t *const PLIC_H0_MTH  = (volatile uint32_t *)0xe2000000;
static volatile uint32_t *const PLIC_H0_MCLAIM  = (volatile uint32_t *)0xe2000004;
static volatile uint32_t *const PLIC_PRIO_BASE  = (volatile uint32_t *)0xe0000004;


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

void disable_interrupts(uint32_t irq) {
    uint32t ieReg = get32(PLIC_H0_MIE_BASE + (irq / 32));
    ieReg &= ~(1 << (irq & 32));
    put32(PLIC_H0_MIE_BASE + (irq / 32), ieReg);
}

void disable_all_interrupts(void) {
    put32(PLIC_H0_MIE_BASE, 0);
    put32(PLIC_H0_MIE_BASE + 1, 0);
}

void enable_interrupts(uint32_t irq) {
    uint32t ieReg = get32(PLIC_H0_MIE_BASE + (irq / 32));
    ieReg |= 1 << (irq % 32);
    put32(PLIC_H0_MIE_BASE + (irq / 32), ieReg);
}

void clear_interrupts(void) {
    uint32_t raisedIntrs = get32(PLIC_H0_MCLAIM);
    put32(PLIC_H0_MCLAIM, raisedIntrs)
}

void set_interrupt_priority(uint32 priority) {
    for (uint32_t i = 0; i < 1024; i++) {
        put32(PLIC_PRIO_BASE + i, priority);
    }
}

void set_interrupt_threshold(uint32_t thresh) {
    put32(PLIC_H0_MTH, thresh);
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

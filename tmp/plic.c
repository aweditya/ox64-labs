#define LOG_LEVEL 3
#include "lib.h"

#define IRQ_NUM_BASE 16 // pg 45 BL808

// PLIC_PRIOx sets the interrupt priority for source x
static volatile uint32_t *const PLIC_PRIO_BASE  = (volatile uint32_t *)0xe0000004;

// PCIC_IPx stores pending state for interrupts 32x to 32(x+1)-1
static volatile uint32_t *const PLIC_IP_BASE    = (volatile uint32_t *)0xe0001000;

// PLIC_H0_MIEx stores interrupt enable for interrupts 32x to 32(x+1)-1
static volatile uint32_t *const PLIC_H0_MIE_BASE = (volatile uint32_t *)0xe0002000;

// M-mode interrupt threshold register
static volatile uint32_t *const PLIC_H0_MTH  = (volatile uint32_t *)0xe2000000;

// M-mode claim/complete register
static volatile uint32_t *const PLIC_H0_MCLAIM  = (volatile uint32_t *)0xe2000004;

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

// pg 89 of c906 doc
static void intr_req_response(void) {
    // 1. read claim/complete reg
    // the read operation resets the ip bit of
    // the corresponding plic
    uint32_t mclaim_reg = get32(PLIC_H0_MCLAIM);
    itoa_hex(mclaim_reg);

    // 2. if mclaim_reg == 0, return
    // im guessing you should send an intr
    // completion msg?
    if (mclaim_reg == 0)
        return;

    // 3. if matches timer interrupt id
    if (mclaim_reg == IRQ_NUM_BASE + 61) {
        uart_puts("Hit timer interrupt!");
    }
}

// pg 89 of c906
static void intr_completion(uint32_t target) {
    // write id of intr to corresponding claim/complete reg
    put32(PLIC_H0_MCLAIM, target);
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

void set_intr_prio(uint32_t irq, unsigned prio) {
    if (prio >= 32)
        // disallowed priority
        return;

    put32(PLIC_PRIO_BASE + irq, prio);
}

void set_intr_th(unsigned th) {
    if (th >= 32)
        // disallowed threshold
        return;

    // intr only triggered if
    // prio of intr > th
    put32(PLIC_H0_MTH, th);
}


void disable_interrupts(uint32_t irq) {
    uint32_t ie_reg = get32(PLIC_H0_MIE_BASE + (irq / 32));
    ie_reg &= ~(1 << (irq & 32));
    put32(PLIC_H0_MIE_BASE + (irq / 32), ie_reg);
}

void disable_all_interrupts(void) {
    // disable all interrupts
    for (int i=0; i<32; i++) {
        put32(PLIC_H0_MIE_BASE + i, 0);
    }
}

void enable_interrupts(uint32_t irq) {
    uint32_t ie_reg = get32(PLIC_H0_MIE_BASE + (irq / 32));
    ie_reg |= 1 << (irq % 32);
    put32(PLIC_H0_MIE_BASE + (irq / 32), ie_reg);
}

void clear_interrupts(void) {
    uint32_t raisedIntrs = get32(PLIC_H0_MCLAIM);
    put32(PLIC_H0_MCLAIM, raisedIntrs)
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

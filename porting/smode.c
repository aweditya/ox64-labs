#include "lib.h"
#include "printk.h"

void init_uart(void) {
    uart_init(UART0, 115200);
}

void trap_vector(void) {
    while(1) {
        uart_puts(UART0, "in trap!\r\n");
        delay_ms(1000);
    }
}

static inline uint64_t read_csr(const char *name) {
    uint64_t val;
    asm volatile("csrr %0, %1" : "=r"(val) : "i"(name));
    return val;
}

// Safer version for hard-coded CSRs:
static inline uint64_t csrr_mstatus() {
    uint64_t val;
    asm volatile("csrr %0, mstatus" : "=r"(val));
    return val;
}
static inline uint64_t csrr_mepc() {
    uint64_t val;
    asm volatile("csrr %0, mepc" : "=r"(val));
    return val;
}
static inline uint64_t csrr_mtvec() {
    uint64_t val;
    asm volatile("csrr %0, mtvec" : "=r"(val));
    return val;
}
static inline uint64_t csrr_mcause() {
    uint64_t val;
    asm volatile("csrr %0, mcause" : "=r"(val));
    return val;
}
static inline uint64_t csrr_mtval() {
    uint64_t val;
    asm volatile("csrr %0, mtval" : "=r"(val));
    return val;
}
static inline uint64_t csrr_mieval() {
    uint64_t val;
    asm volatile("csrr %0, mie" : "=r"(val));
    return val;
}
static inline uint64_t csrr_mipval() {
    uint64_t val;
    asm volatile("csrr %0, mip" : "=r"(val));
    return val;
}

static inline uint64_t csrr_satpval() {
    uint64_t val;
    asm volatile("csrr %0, satp" : "=r"(val));
    return val;
}

void print_in_trap(void) {
    uart_puts(UART0, "inside trap\n");
}

void dump_csrs(void) {
    uart_puts(UART0, "dumping csrs...\n");
    uart_puts(UART0, "mstatus: "); uart_puthex64(csrr_mstatus()); uart_putc(UART0, '\n');
    uart_puts(UART0, "mepc:    "); uart_puthex64(csrr_mepc());    uart_putc(UART0, '\n');
    uart_puts(UART0, "mtvec:    "); uart_puthex64(csrr_mtvec());    uart_putc(UART0, '\n');
    uart_puts(UART0, "mcause:  "); uart_puthex64(csrr_mcause());  uart_putc(UART0, '\n');
    uart_puts(UART0, "mtval:   "); uart_puthex64(csrr_mtval());   uart_putc(UART0, '\n');
    uart_puts(UART0, "mie:   "); uart_puthex64(csrr_mieval());   uart_putc(UART0, '\n');
    uart_puts(UART0, "mip:   "); uart_puthex64(csrr_mipval());   uart_putc(UART0, '\n');
    uart_puts(UART0, "satp:   "); uart_puthex64(csrr_satpval());   uart_putc(UART0, '\n');
}

void kmain(void) {
    init_uart();
    uart_puts(UART0, "hello, main!\r\n");

    while(1) {
        uart_puts(UART0, "inside the while loop\r\n");
        dump_csrs();
        uart_puts(UART0, "hello, world!\r\n");
        delay_ms(1000);
    }
}

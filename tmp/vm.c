#define LOG_LEVEL 3
#include "lib.h"
#include "assert.h"

#define IRQ_NUM_BASE 16 // pg 45 BL808

// Helper function to convert an integer to a hexadecimal string and print using putc
static void itoa_hex(uint64_t num) {
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

void put32_check(volatile uint32_t *addr, uint32_t v) {
    uart_puts(UART0, "Writing to address: ");
    itoa_hex((uint64_t)addr);
    put32(addr, v);
    uart_puts(UART0, "Finished writing\n");
    if (get32(addr) != v) {
        uart_puts(UART0, "Write failed\n");
    } else {
        uart_puts(UART0, "Write succeeded\n");
    }
}

void PUT32_check(uint64_t addr, uint32_t v) {
    uart_puts(UART0, "Writing to address: ");
    itoa_hex(addr);
    PUT32(addr, v);
    uart_puts(UART0, "Finished writing\n");
    if (GET32(addr) != v) {
        uart_puts(UART0, "Write failed\n");
    } else {
        uart_puts(UART0, "Write succeeded\n");
    }
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

// pg 637 of the c906 doc
static inline uint64_t get_mapbaddr(void) {
    uint64_t result;
    asm volatile("csrr %0, mapbaddr" : "=r"(result));
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

_attribute__((aligned(4))) void handler(void) {
    uart_puts(UART0, "Inside handler\n");
    uint32_t stat = GET32(timer1_match_status);
    itoa_hex(stat);

    uint64_t mepc;
    asm volatile("csrr %0, mepc" : "=r"(mepc));

    uint64_t mcause;
    asm volatile("csrr %0, mcause" : "=r"(mcause));

    uint64_t mtval;
    asm volatile("csrr %0, mtval" : "=r"(mtval));

    uart_puts(UART0, "MEPC: ");
    itoa_hex(mepc);
    uart_putc(UART0, '\n');
    uart_puts(UART0, "MCAUSE: ");
    itoa_hex(mcause);
    uart_putc(UART0, '\n');
    uart_puts(UART0, "MTVAL: ");
    itoa_hex(mtval);
    uart_putc(UART0, '\n');

    // REMOVE: move back above mepc line 247??
    uint32_t target = intr_req_response();
    uart_puts(UART0, "target: ");
    itoa_hex((uint64_t)target);
    if (target == 0)
        return;

    intr_completion(target);
}

void disable_interrupts(void) {
    asm volatile("csrci mstatus, 0x8");
}

void enable_interrupts(void) {
    asm volatile("csrsi mstatus, 0x8");
}


typedef struct {
    uint32_t ppn    : 28;
    uint32_t rsvd1  : 16;
    uint32_t asid   : 16;
    uint32_t mode   : 4;
} satp_t;

// Pg. 65ish
typedef struct {
    uint32_t valid  : 1;
    uint32_t exec   : 1; // executable
    uint32_t user   : 1; 
    uint32_t glbl   : 1;
    uint32_t accsd  : 1; // accessed
    uint32_t dirty  : 1;
    uint32_t rsw    : 2; // 
    uint32_t ppn0   : 9;
    uint32_t ppn1   : 9;
    uint32_t ppn2   : 10;
    uint32_t rsvd   : 21;
    uint32_t undef  : 1;
    uint32_t rsvd   : 1;
    uint32_t buff   : 1; // bufferable
    uint32_t c      : 1; // cacheable (if we want to enable caching might be necessary)
    uint32_t so     : 1; // strong order (access order required by memory)
} pte_t;
_Static_assert(sizeof(pte_t) == 8, "darn. pte_t not 8 bytes");

static inline void write_satp(uint64_t value) {
    asm volatile("csrw satp, %0" :: "r"(value));
    asm volatile("sfence.vma"); // flush tlb
}

static inline uint64_t read_satp(void) {
    uint64_t result;
    asm volatile ("sfence.vma"); // flush tlb (necessary?)
    asm volatile ("csrr satp, %0" : "=r"(result));
    return result;
}

// Page 64
bool mmu_is_enabled(void) {
    // check that mode is 8
    return (read_satp & (0xF << 60)) == 0x8;
}
bool mmu_is_disabled(void) {
    // check that mode is 0
    return (read_satp & (0xF << 60)) == 0;
}

// enable mmu
void mmu_enable(void) {
    uint64_t value = read_satp();
    value |= 0x8 << 60; // mode = 8
    write_satp(value);
}

// set base ppn
void mmu_set_base_ppn(uint64_t ppn) {
    if (ppn & 0xFFFFFFF != ppn) {
        uart_puts(UART0, "base ppn should be a 28 bit number");
    }
    uint64_t v = read_satp();
    value &= ~0xFFFFFFF; // mask out first 28 bits
    value |= ppn; // load pnp into first 28 bits
}

// set asid
void 

// Initialize mmu
void mmu_init(void) {
    if (mmu_is_enabled()) {
        uart_puts(UART0, "mmu should be disabled?");
    }
     
    mmu_enable();
}

void kmain(void) {
  uart_init(UART0, 115200);

  while(1) {
  }
}

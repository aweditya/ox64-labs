#define LOG_LEVEL 3
#include "lib.h"
// #include "assert.h"

#define IRQ_NUM_BASE 16 // pg 45 BL808

#define PT_SIZE 512

#define ASID    1LL
#define BARE    0LL
#define Sv39    8LL
#define Sv48    9LL
#define Sv57    10LL
#define Sv64    11LL

uint64_t pg1[PT_SIZE] __attribute__((section(".pg1")));
uint64_t pg2[PT_SIZE] __attribute__((section(".pg2")));
uint64_t pg3[PT_SIZE] __attribute__((section(".pg3")));

void init_uart(void) {
    uart_init(UART0, 115200);
}

void print_in_trap(void) {
    uart_puts(UART0, "Inside trap handler\n");
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

void dump_csrs(void) {
    uart_puts(UART0, "Dumping CSRs...\n");
    uart_puts(UART0, "mstatus:\t"); uart_puthex64(csrr_mstatus()); uart_putc(UART0, '\n');
    uart_puts(UART0, "mepc:\t"); uart_puthex64(csrr_mepc());    uart_putc(UART0, '\n');
    uart_puts(UART0, "mtvec:\t"); uart_puthex64(csrr_mtvec());    uart_putc(UART0, '\n');
    uart_puts(UART0, "mcause:\t"); uart_puthex64(csrr_mcause());  uart_putc(UART0, '\n');
    uart_puts(UART0, "mtval:\t"); uart_puthex64(csrr_mtval());   uart_putc(UART0, '\n');
    uart_puts(UART0, "mie:\t"); uart_puthex64(csrr_mieval());   uart_putc(UART0, '\n');
    uart_puts(UART0, "mip:\t"); uart_puthex64(csrr_mipval());   uart_putc(UART0, '\n');
    uart_puts(UART0, "satp:\t"); uart_puthex64(csrr_satpval());   uart_putc(UART0, '\n');
}

void put32_check(volatile uint32_t *addr, uint32_t v) {
    uart_puts(UART0, "Writing to address: ");
    uart_puthex64((uint64_t)addr);
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
    uart_puthex64(addr);
    PUT32(addr, v);
    uart_puts(UART0, "Finished writing\n");
    if (GET32(addr) != v) {
        uart_puts(UART0, "Write failed\n");
    } else {
        uart_puts(UART0, "Write succeeded\n");
    }
}

static inline void write_satp(uint64_t value) {
    uart_puts(UART0, "Trying to write to SATP...\n");
    asm volatile(
            "csrw satp, %0" 
            : 
            : "r"(value));
    // Flush TLB after writing.
    asm volatile("sfence.vma");

}

static inline uint64_t read_satp(void) {
    uint64_t result;
    // Flush TLB before reading
    asm volatile ("sfence.vma");
    asm volatile (
            "csrr %0, satp" 
            : "=r"(result)
            :);
    return result;
}

// Page 64
bool mmu_is_enabled(void) {
    // check that mode is 8
    uint64_t v = read_satp();
    return (v >> 60) == 0x8;
}

bool mmu_is_disabled(void) {
    // check that mode is 0
    uint64_t v = read_satp();
    return (v >> 60) == 0x0;
}

// Enable MMU
void mmu_enable(void) {
    uint64_t value = read_satp();
    value &= 0x0fffffffffffffff;
    value |= Sv39 << 60; // mode = 8
    write_satp(value);
}

// Disable MMU
void mmu_disable(void) {
    uint64_t value = read_satp();
    value = 0;
    write_satp(value);
}

// Initialize MMU
void mmu_init(void) {
    if (mmu_is_enabled()) {
        uart_puts(UART0, "MMU should be disabled?");
    }
     
    mmu_enable();
}

void kmain(void) {
    uart_puts(UART0, "Inside kmain\n");
  
    if (mmu_is_disabled()) {
      uart_puts(UART0, "MMU is disabled to begin as expected\n");
    }
  
    uart_puts(UART0, "Printing the address of the page tables\n");
    uart_puts(UART0, "pg1:\t"); uart_puthex64((uint64_t)pg1); uart_putc(UART0, '\n');
    uart_puts(UART0, "pg2:\t"); uart_puthex64((uint64_t)pg2); uart_putc(UART0, '\n');
    uart_puts(UART0, "pg3:\t"); uart_puthex64((uint64_t)pg3); uart_putc(UART0, '\n');
  
    // 1. Write our page_table addr into satp
    uart_puts(UART0, "Printing the value we want to populate in the SATP register\n");
    uint64_t satp = (BARE << 60) | (ASID << 44) | ((uint64_t)pg1 >> 12);
    uart_puts(UART0, "satp:\t"); uart_puthex64(satp); uart_putc(UART0, '\n');
    write_satp(satp);
    uart_puts(UART0, "Finished writing to SATP\n");
  
    if (read_satp() == satp) {
        uart_puts(UART0, "Write to SATP went through as expected!\n");
    } else {
        uart_puts(UART0, "Write to SATP failed?\n");
    }

    uart_puts(UART0, "Manually setting up an identity page table mapping!\n");
    // all pages
    // level 1
    for (uint64_t vpn2 = 0; vpn2 < 1<<9; vpn2++) {
        // uart_puts(UART0, "vpn2: ");
        // uart_puthex64(vpn2);
        // pg1, pg2, pg3 are already 12-bit aligned
        uint64_t idx_l1 = (uint64_t)(pg1)|(vpn2<<3) - (uint64_t)(pg1);
        pg1[idx_l1] = (((uint64_t)pg2)<<10) | 1<<0;
  
        uart_puts(UART0, "level1 mapping done for: "); uart_puthex64(vpn2); uart_putc(UART0, '\n');
  
        // level 2
        for (uint64_t vpn1 = 0; vpn1 < 1<<9; vpn1++) {
            uint64_t idx_l2 = (uint64_t)(pg2)|(vpn1<<3) - (uint64_t)(pg2);
            pg2[idx_l2] = (((uint64_t)pg3)<<10) | 1<<0;
  
            // level 3
            for (uint64_t vpn0 = 0; vpn0 < 1<<9; vpn0++) {
                uint64_t idx_l3 = (uint64_t)(pg3)|(vpn0<<3) - (uint64_t)(pg3);
                uint64_t ppn = (vpn2<<18)|(vpn1<<9)|(vpn0);
                // leaf reached
                pg3[idx_l3] = (ppn<<10) | 1<<0 | 0b111 << 1;
            }
        }
    }

    while (1) {
        uart_puts(UART0, "Hello, world!\n");
        asm volatile("wfi");
    }

//   
//     uart_puts(UART0, "Enabling MMU!\n");
//     mmu_enable();
//   
//     // check mmu is enabled
//     if (read_satp() >> 60 == Sv39) {
//         uart_puts(UART0, "MMU successfully enabled!\n");
//     }
//   
//     // check we can put/get from address with mmu enabled 
//     // (this all works as of right now)
//     uint32_t cookie = 0xdeadbeef;
//     if (get32(&cookie) == 0xdeadbeef) {
//         uart_puts(UART0, "get works\n");
//     }
//     uart_puts(UART0, "trying to put\n");
//     put32(&cookie, 0xbeefbabe);
//     if (get32(&cookie) == 0xbeefbabe) {
//         uart_puts(UART0, "put works!!!!\n");
//     } else {
//         uart_puts(UART0, "put no work...\n");
//     }
//   
//     while(1) {
//       uart_puts(UART0, "...");
//       delay_ms(1000);
//     }
}

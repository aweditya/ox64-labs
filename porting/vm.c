#define LOG_LEVEL 3
#include "lib.h"
// #include "assert.h"

#define IRQ_NUM_BASE 16 // pg 45 BL808

#define PT_SIZE     1<<9
#define PGOFF       1<<12
#define PTESIZE     8
#define VPN_BITS    9

#define ASID    1LL
#define BARE    0LL
#define Sv39    8LL
#define Sv48    9LL
#define Sv57    10LL
#define Sv64    11LL

uint64_t pg1[PT_SIZE] __attribute__((aligned(4096)));
uint64_t pg2[PT_SIZE] __attribute__((aligned(4096)));
uint64_t pg3[PT_SIZE][PT_SIZE] __attribute__((aligned(4096)));


#define PSRAM_START 0x50000000
#define PSRAM_SZ    64*1024*1024
#define PSRAM_END   PSRAM_START+PSRAM_SZ

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
    // asm volatile("sfence.vma");
}

static inline uint64_t read_satp(void) {
    uint64_t result;
    // Flush TLB before reading
    // asm volatile ("sfence.vma");
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
    return (v >> 60) == Sv39;
}

bool mmu_is_disabled(void) {
    // check that mode is 0
    uint64_t v = read_satp();
    return (v >> 60) == BARE;
}

// Enable MMU
void mmu_enable(uint64_t pg1_base) {
    uint64_t satp = (Sv39 << 60) | (ASID << 44) | (pg1_base >> 12);
    write_satp(satp);
}

// Disable MMU
void mmu_disable(void) {
    write_satp(0);
}

void check_identity_mapping(uint64_t va) {
    uart_puts(UART0, "Checking mapping for VA = ");
    uart_puthex64(va); uart_putc(UART0, '\n');

    uint64_t vpn0 = (va >> 12) & 0x1FF;
    uint64_t vpn1 = (va >> 21) & 0x1FF;
    uint64_t vpn2 = (va >> 30) & 0x1FF;

    uart_puts(UART0, "VPN2: "); uart_puthex64(vpn2); uart_putc(UART0, '\n');
    uart_puts(UART0, "VPN1: "); uart_puthex64(vpn1); uart_putc(UART0, '\n');
    uart_puts(UART0, "VPN0: "); uart_puthex64(vpn0); uart_putc(UART0, '\n');

    uint64_t pte2 = pg1[vpn2];
    if (!(pte2 & 0x1)) {
        uart_puts(UART0, "Level 2 PTE invalid!\n");
        uart_puthex64(pte2); uart_putc(UART0, '\n');
        return;
    }
    uint64_t *l2 = (uint64_t *)((pte2 >> 10) << 12);

    uint64_t pte1 = l2[vpn1];
    if (!(pte1 & 0x1)) {
        uart_puts(UART0, "Level 1 PTE invalid!\n");
        uart_puthex64(pte1); uart_putc(UART0, '\n');
        return;
    }
    uint64_t *l1 = (uint64_t *)((pte1 >> 10) << 12);

    uint64_t pte0 = l1[vpn0];
    if (!(pte0 & 0x1)) {
        uart_puts(UART0, "Level 0 PTE invalid!\n");
        uart_puthex64(pte0); uart_putc(UART0, '\n');
        return;
    }

    // Leaf PTE: extract physical page number
    uint64_t ppn = pte0 >> 10;
    uint64_t pa = (ppn << 12) | (va & 0xFFF);

    uart_puts(UART0, "Translated PA = ");
    uart_puthex64(pa); uart_putc(UART0, '\n');

    if (pa == va) {
        uart_puts(UART0, "✅ Identity mapping verified!\n");
    } else {
        uart_puts(UART0, "❌ Mapping incorrect!\n");
    }
}

void kmain(void) {
    uart_puts(UART0, "Inside kmain\n");
  
    if (mmu_is_disabled()) {
      uart_puts(UART0, "MMU is disabled to begin as expected\n");
    }
  
    uart_puts(UART0, "Printing the address of the page tables\n");
    uint64_t pg1_base = (uint64_t)pg1;
  
    // 1. Write our page_table addr into satp
    uart_puts(UART0, "Printing the value we want to populate in the SATP register\n");
    uint64_t satp = (Sv39 << 60) | (ASID << 44) | (pg1_base >> 12);
    uart_puts(UART0, "satp:\t"); uart_puthex64(satp); uart_putc(UART0, '\n');

    uart_puthex64((uint64_t)pg1); uart_putc(UART0, '\n');
    uart_puthex64((uint64_t)pg2); uart_putc(UART0, '\n');
    uart_puthex64((uint64_t)pg3); uart_putc(UART0, '\n');

//     while (1) {
//         uart_puts(UART0, "Hello, world!\n");
//         asm volatile("wfi");
//     }


    uart_puts(UART0, "Manually setting up an identity page table mapping!\n");
    uart_puthex64(PSRAM_START); uart_putc(UART0, '\n');
    uart_puthex64(PSRAM_END); uart_putc(UART0, '\n');
    for (uint64_t addr = PSRAM_START; addr < PSRAM_END; addr+=PGOFF) {
        uart_puts(UART0, "Setting up mapping for addr="); uart_puthex64(addr); uart_putc(UART0, '\n');
        uint64_t x = addr;
        uint64_t vpn0 = (x >> 12) & 0x1ff;
        x = x >> 12;

        // Identity mapping
        uint64_t ppn = x;

        uint64_t vpn1 = (x >> VPN_BITS) & 0x1ff;
        x = x >> VPN_BITS;

        uint64_t vpn2 = (x >> VPN_BITS) & 0x1ff;
        x = x >> VPN_BITS;

        // Set R,W,X to be 111 (leaf node)
        // V = 1
        pg3[vpn1][vpn0] = (ppn << 10) | 0b111 << 1 | 1 << 0;

        // Set R,W,X to be 000 (tree node which is not a leaf)
        // V = 1
        uint64_t pg3_base = (uint64_t)pg3[vpn1];
        pg2[vpn1] = ((pg3_base >> 12) << 10) | 0b000 << 1 | 1 << 0;

        // Set R,W,X to be 000 (tree node which is not a leaf)
        // V = 1
        uint64_t pg2_base = (uint64_t)pg2;
        pg1[vpn2] = ((pg2_base >> 12) << 10) | 0b000 << 1 | 1 << 0;
        uart_puts(UART0, "Some metadata\n");
        uart_puthex64(vpn2); uart_putc(UART0, '\n');
        uart_puthex64(vpn1); uart_putc(UART0, '\n');
        uart_puthex64(vpn0); uart_putc(UART0, '\n');
    }

    check_identity_mapping(0x50000000);
    // check_identity_mapping(0x51000000);
    // check_identity_mapping(0x53FFFFF0);

    uart_puts(UART0, "Enabling MMU!\n");
    mmu_enable(pg1_base);
  
    // check mmu is enabled
    if (mmu_is_enabled()) {
        uart_puts(UART0, "MMU successfully enabled!\n");
    }
  
    while (1) {
        uart_puts(UART0, "Hello, world!\n");
        asm volatile("wfi");
    }
}

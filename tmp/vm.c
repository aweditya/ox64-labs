#define LOG_LEVEL 3
#include "lib.h"
#include "assert.h"

#define IRQ_NUM_BASE 16 // pg 45 BL808

#define PT_SIZE 512

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

__attribute__((aligned(4))) void handler(void) {
    uart_puts(UART0, "Inside handler\n");

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
}

void disable_interrupts(void) {
    asm volatile("csrci mstatus, 0x8");
}

void enable_interrupts(void) {
    asm volatile("csrsi mstatus, 0x8");
}


typedef struct {
    uint64_t ppn    : 28;
    uint64_t rsvd1  : 16;
    uint64_t asid   : 16;
    uint64_t mode   : 4;
} __attribute__((packed)) satp_t;

// Pg. 65ish
typedef struct {
    uint64_t valid  : 1;
    uint64_t read   : 1;
    uint64_t write  : 1;
    uint64_t exec   : 1; // executable
    uint64_t user   : 1; 
    uint64_t glbl   : 1;
    uint64_t accsd  : 1; // accessed
    uint64_t dirty  : 1;
    uint64_t rsw    : 2; // 
    uint64_t ppn0   : 9;
    uint64_t ppn1   : 9;
    uint64_t ppn2   : 10;
    uint64_t rsvd1  : 21;
    uint64_t undef  : 1;
    uint64_t rsvd2  : 1;
    uint64_t buff   : 1; // bufferable
    uint64_t c      : 1; // cacheable (if we want to enable caching might be necessary)
    uint64_t so     : 1; // strong order (access order required by memory)
} __attribute__((packed)) pte_t;
_Static_assert(sizeof(pte_t) == 8, "darn. pte_t not 8 bytes");

static inline void write_satp(satp_t value) {
    asm volatile("csrw satp, %0" :: "r"(value));
    asm volatile("sfence.vma"); // flush tlb
}

static inline satp_t read_satp(void) {
    satp_t result;
    asm volatile ("sfence.vma"); // flush tlb (necessary?)
    asm volatile ("csrr satp, %0" : "=r"(result));
    return result;
}

// Page 64
bool mmu_is_enabled(void) {
    // check that mode is 8
    satp_t v = read_satp();
    return v.mode == 0x8;
}
bool mmu_is_disabled(void) {
    // check that mode is 0
    satp_t v = read_satp();
    return v.mode == 0;
}

// enable mmu
void mmu_enable(void) {
    satp_t value = read_satp();
    value.mode = 8; // mode = 8
    write_satp(value);
}

// disable mmu
void mmu_disable(void) {
    satp_t value = read_satp();
    value.mode = 0; // mode = 0
    write_satp(value);
}

// set base ppn
void mmu_set_base_ppn(uint64_t ppn) {
    if (ppn & 0xFFFFFFF != ppn) {
        uart_puts(UART0, "base ppn should be a 28 bit number");
    }
    satp_t v = read_satp();
    v.ppn = ppn; // mask out first 28 bits
    write_satp(v);
}

// set asid
void mmu_set_ctx(uint64_t asid) {
    if (asid & 0xFFFF != asid) {
        uart_puts(UART0, "asid should be a 16 bit number");
    }
  satp_t v = read_satp();
  v.asid = asid;
}


// create pte at base ppn
// leaf pte, so this is a 1G page
void create_pte_level1_leaf(pte_t *pt, volatile uint64_t ppn, uint16_t offset) {
  if ((ppn & 0x3FF) != ppn) {
    uart_puts(UART0, "ppn should be 10 bits for level 1 pte\n");
  }
  if ((offset & 0x1FF) != offset) { 
    uart_puts(UART0, "offset should be < 512\n");
  }

  pte_t v = {0};
  v.valid = 1;
  v.read = 1;
  v.write = 1;
  v.exec = 1; // make executable
  v.user = 1;
  v.glbl = 1;
  v.accsd = 1; // o/w page fault ? pg. 66
  v.dirty = 1; // allows writes (dirty)
  v.ppn2 = ppn;
  v.buff = 1;
  v.c = 1;
  v.so = 1; // should it be strongly ordered
  pt[offset] = v;
}

/*
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
*/


// Initialize mmu
void mmu_init(void) {
    if (mmu_is_enabled()) {
        uart_puts(UART0, "mmu should be disabled?");
    }
     
    mmu_enable();
}

void kmain(void) {
  uart_init(UART0, 115200);
  
  if (mmu_is_disabled()) {
    uart_puts(UART0, "mmu is disabled to begin as expected\n");
  }

  // initialize base pt on stack
  pte_t pt[PT_SIZE];
  
  // volatile to not optimize out writes
  volatile uint64_t *ppn = (volatile uint64_t *)0x51000000;
  uint16_t offset = 256;
  // TODO: change this to take desired virtual -> physical addr mapping
  create_pte_level1_leaf(pt, (uint64_t)ppn, offset);
  
  // write to ppn, should map to va
  *ppn = 0xdeadbeef;

  // Enable virtual memory, and check that virtual address holds deadbeef
  volatile uint64_t va = (256 << 30 ) /* VPN[2] */| (0 << 21) /*VPN[1] */ | (0 << 12) /* VPN[0] */| (0x000);

  enum { ASID = 1 };
  mmu_set_ctx(ASID);
  mmu_set_base_ppn((uint64_t)&pt[0]);
  mmu_enable();

  uart_puts(UART0, "value at va should be deafbeef\n");

  itoa_hex(*va);

  while(1) {
    uart_puts(UART0, "...");
    delay_ms(1000);
  }
}

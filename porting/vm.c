#define LOG_LEVEL 3
#include "lib.h"
#include "assert.h"

#define IRQ_NUM_BASE 16 // pg 45 BL808

#define PT_SIZE 512

uint64_t pg1[PT_SIZE] __attribute__((section(".pg1")));
uint64_t pg2[PT_SIZE] __attribute__((section(".pg2")));
uint64_t pg3[PT_SIZE] __attribute__((section(".pg3")));

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

static inline void write_satp(uint64_t value) {
    asm volatile(
            "csrw satp, %0" 
            : 
            : "r"(value));
    asm volatile("sfence.vma"); // flush tlb
}

static inline uint64_t read_satp(void) {
    uint64_t result;
    asm volatile ("sfence.vma"); // flush tlb (necessary?)
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

// enable mmu
void mmu_enable(void) {
    uint64_t value = read_satp();
    value &= 0x0fffffffffffffff;
    value |= 8LL << 60; // mode = 8
    write_satp(value);
}

// disable mmu
void mmu_disable(void) {
    uint64_t value = read_satp();
    value &= 0x0fffffffffffffff;
    write_satp(value);
}

// set base ppn
void mmu_set_base_ppn(uint64_t ppn) {
    if (ppn & 0xFFF != ppn) {
        uart_puts(UART0, "last 12 bits of ppn must be 0");
    }
    uint64_t v = read_satp();
    v &= ~(0xfffffff); // mask out first 28 bits
    v |= ppn >> 12;
    write_satp(v);
}

// set asid
void mmu_set_ctx(uint64_t asid) {
    if (asid & 0xffff != asid) {
        uart_puts(UART0, "asid should be a 16 bit number");
    }
  uint64_t v = read_satp();
  v &= 0xf0000fffffffffff;
  v |= asid << 44;
  write_satp(v);
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

  disable_interrupts();
  
  if (mmu_is_disabled()) {
    uart_puts(UART0, "mmu is disabled to begin as expected\n");
  }

  vector_base_set((uint64_t *)handler);
  enable_interrupts();
#if 0
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
  volatile uint64_t va = (uint64_t)256 << 30;

  enum { ASID = 1 };
  mmu_set_ctx(ASID);
  mmu_set_base_ppn((uint64_t)&pt[0]);
  mmu_enable();

  uart_puts(UART0, "value at va should be deafbeef\n");

  itoa_hex(va);
#endif

  itoa_hex((uint64_t)pg1);
  itoa_hex((uint64_t)pg2);
  itoa_hex((uint64_t)pg3);

  // 1. write our page_table addr into satp
  enum { ASID = 1LL };
  uint64_t satp = (8LL<<60) | (ASID*1LL<<44) | ((uint64_t)pg1>>12);
  itoa_hex(satp);
  write_satp(satp);
  uart_puts(UART0, "Finished writing to SATP\n");
  

  if (read_satp() == satp) {
      uart_puts(UART0, "Write went through as expected!\n");
  }

  uart_puts(UART0, "Setting up mapping!\n");
  // all pages
  // level 1
  for (uint64_t vpn2 = 0; vpn2 < 1<<9; vpn2++) {
      // uart_puts(UART0, "vpn2: ");
      // itoa_hex(vpn2);
      // pg1, pg2, pg3 are already 12-bit aligned
      uint64_t idx_l1 = (uint64_t)(pg1)|(vpn2<<3) - (uint64_t)(pg1);
      pg1[idx_l1] = (((uint64_t)pg2)<<10) | 1<<0;

      uart_puts(UART0, "level1 mapping done for: ");
      itoa_hex(vpn2);

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

  uart_puts(UART0, "Enabling MMU!\n");
  mmu_enable();

  // check mmu is enabled
  if (read_satp() >> 60 == 8) {
      uart_puts(UART0, "MMU successfully enabled!\n");
  }

  // check we can put/get from address with mmu enabled 
  // (this all works as of right now)
  uint32_t cookie = 0xdeadbeef;
  if (get32(&cookie) == 0xdeadbeef) {
      uart_puts(UART0, "get works\n");
  }
  uart_puts(UART0, "trying to put\n");
  put32(&cookie, 0xbeefbabe);
  if (get32(&cookie) == 0xbeefbabe) {
      uart_puts(UART0, "put works!!!!\n");
  } else {
      uart_puts(UART0, "put no work...\n");
  }

  while(1) {
    uart_puts(UART0, "...");
    delay_ms(1000);
  }
}

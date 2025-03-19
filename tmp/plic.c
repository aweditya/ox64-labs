#define LOG_LEVEL 3
#include "lib.h"
#include "assert.h"

#define IRQ_NUM_BASE 16 // pg 45 BL808

enum {
    timer_clk_src           = 0x2000a500,

    timer0_match_val0       = 0x2000a510,
    timer0_match_val1       = 0x2000a514,
    timer0_match_val2       = 0x2000a518,
    timer1_match_val0       = 0x2000a51c,
    timer1_match_val1       = 0x2000a520,
    timer1_match_val2       = 0x2000a524,

    timer0_ctr_val          = 0x2000a52c,
    timer1_ctr_val          = 0x2000a530,

    timer0_match_status     = 0x2000a538,
    timer1_match_status     = 0x2000a53c,

    timer0_match_intr_en    = 0x2000a544,
    timer1_match_intr_en    = 0x2000a548,

    timer0_preload_val      = 0x2000a550,
    timer1_preload_val      = 0x2000a554,

    timer0_preload_ctl      = 0x2000a55c,
    timer1_preload_ctl      = 0x2000a560,

    timer0_intr_clr         = 0x2000a578,
    timer1_intr_clr         = 0x2000a57c,

    timer_ctr_en_clr        = 0x2000a584,

    timer_ctr_mode          = 0x2000a588,

    timer0_intr_level       = 0x2000a590,
    timer1_intr_level       = 0x2000a594,

    timer_clk_div           = 0x2000a5bc,
};

typedef struct {
    uint32_t clk_src_timer0     : 4;
    uint32_t clk_src_timer3     : 4;
    uint32_t clk_src_wdt        : 4;
    uint32_t rsvd               : 4;
    uint32_t timer_rsvd         : 8;
    uint32_t id                 : 8;
} __attribute__((packed)) tccr_t;

typedef struct {
    uint32_t rsvd1          : 1;
    uint32_t timer0_en      : 1;
    uint32_t timer3_en      : 1;
    uint32_t rsvd2          : 2;
    uint32_t timer0_cnt_clr : 1;
    uint32_t timer3_cnt_clr : 1;
    uint32_t rsvd3          : 25;
} __attribute__((packed)) tcer_t;

typedef struct {
    uint32_t rsvd               : 8;
    uint32_t timer0_clk_div_val : 8;
    uint32_t timer3_clk_div_val : 8;
    uint32_t wdt_clk_div_val    : 8;
} __attribute__((packed)) tcdr_t;

enum {
    SRC_FCLK    = 0,
    SRC_32K     = 1,
    SRC_1K      = 2,
    SRC_BCLK    = 3,
    SRC_GPIO    = 4,
    SRC_NOCLK   = 5,
};

enum {
    NO_PRELOAD = 0,
    MATCH_CMP0 = 1,
    MATCH_CMP1 = 2,
    MATCH_CMP2 = 3,
};

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

// initialize timer0 with one match/comparator value
void timer0_init(uint32_t match_val0, uint32_t match_val1, uint32_t match_val2) {
    // Disable timer0 
    uint32_t tcer = GET32(timer_ctr_en_clr);
    tcer &= ~(1 << 1);
    PUT32_check(timer_ctr_en_clr, tcer);

    // clear interrupts
    PUT32(timer0_intr_clr, 0b111);

    // set mode to preload (clear bit 1)
    uint32_t tcmr = GET32(timer_ctr_mode);
    tcmr &= ~(1 << 1);
    PUT32_check(timer_ctr_mode, tcmr);

    // set preload value to 0
    PUT32_check(timer0_preload_val, 0);

    // set preload mode to match on comparator on the highest
    // assumption match_val0 < match_val1 < match_val2
    PUT32_check(timer0_preload_ctl, MATCH_CMP2);

    // set comparator match values 
    PUT32_check(timer0_match_val0, match_val0);
    PUT32_check(timer0_match_val1, match_val1);
    PUT32_check(timer0_match_val2, match_val2);

    // set clock source, page 482
    // Set clock to f32k
    uint32_t x = GET32(timer_clk_src);
    // mask out bits 3:0
    x &= ~(0b1111 << 0);
    x |= SRC_32K << 0;
    PUT32_check(timer_clk_src, x);

    // setup the clock divider. For now write 0 - no freq div
    // 1 - div by 2
    // and so on...
    uint32_t tcdr = GET32(timer_clk_div);
    tcdr &= ~(0xff << 8);
    PUT32_check(timer_clk_div, tcdr);

    // enable timer1 interrupts for match val 0
    PUT32_check(timer0_match_intr_en, 0b111);
}

void timer0_start(void) {
    // Enable timer0 by writing 1 to bit 1
    uint32_t tcer = GET32(timer_ctr_en_clr);
    tcer |= 1 << 1;
    PUT32_check(timer_ctr_en_clr, tcer);
}


// initialize timer1 with one match/comparator value
void timer1_init(uint32_t match_val0, uint32_t match_val1, uint32_t match_val2) {
    // Disable timer1 
    uint32_t tcer = GET32(timer_ctr_en_clr);
    tcer &= ~(1 << 2);
    PUT32_check(timer_ctr_en_clr, tcer);

    // clear interrupts
    PUT32(timer1_intr_clr, 0b111);

    // set mode to preload (clear bit 2)
    uint32_t tcmr = GET32(timer_ctr_mode);
    tcmr &= ~(1 << 2);
    PUT32_check(timer_ctr_mode, tcmr);

    // set preload value to 0
    PUT32_check(timer1_preload_val, 0);

    // set preload mode to match on comparator on the highest
    // assumption match_val0 < match_val1 < match_val2
    PUT32_check(timer1_preload_ctl, MATCH_CMP2);

    // set comparator match values 
    PUT32_check(timer1_match_val0, match_val0);
    PUT32_check(timer1_match_val1, match_val1);
    PUT32_check(timer1_match_val2, match_val2);

    // set clock source, page 482
    // Set clock to f32k
    uint32_t x = GET32(timer_clk_src);
    // mask out bits 7:4
    x &= ~(0b1111 << 4);
    x |= SRC_32K << 4;
    PUT32_check(timer_clk_src, x);

    // setup the clock divider. For now write 0 - no freq div
    // 1 - div by 2
    // and so on...
    uint32_t tcdr = GET32(timer_clk_div);
    tcdr &= ~(0xff << 16);
    PUT32_check(timer_clk_div, tcdr);

    // enable timer1 interrupts for match val 0
    PUT32_check(timer1_match_intr_en, 0b111);
}

void timer1_start(void) {
    // Enable timer1 by writing 1 to bit 2
    uint32_t tcer = GET32(timer_ctr_en_clr);
    tcer |= 1 << 2;
    PUT32_check(timer_ctr_en_clr, tcer);
}

// PLIC_PRIOx sets the interrupt priority for source x
#define PLIC_PRIO_BASE      0x0000004

// PCIC_IPx stores pending state for interrupts 32x to 32(x+1)-1
#define PLIC_IP_BASE        0x0001000

// PLIC_H0_MIEx stores interrupt enable for interrupts 32x to 32(x+1)-1
#define PLIC_H0_MIE_BASE    0x0002000

// M-mode interrupt threshold register
#define PLIC_H0_MTH         0x0200000

// M-mode claim/complete register
#define PLIC_H0_MCLAIM      0x0200004

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

// pg 89 of c906 doc
static uint32_t intr_req_response(void) {
    uint64_t mapbaddr = get_mapbaddr();

    // 1. read claim/complete reg
    // the read operation resets the ip bit of
    // the corresponding plic
    uint32_t *plic_h0_mclaim = (uint32_t *)(PLIC_H0_MCLAIM + mapbaddr);
    uint32_t mclaim_reg = get32(plic_h0_mclaim);
    itoa_hex(mclaim_reg);

    // 2. if mclaim_reg == 0, return
    // im guessing you should send an intr
    // completion msg?
    if (mclaim_reg == 0)
        return 0;

    // 3. if matches timer interrupt id
    if (mclaim_reg == IRQ_NUM_BASE + 61 || mclaim_reg == IRQ_NUM_BASE + 62) {
        uart_puts(UART0, "Hit timer interrupt!\n");
    }

    return mclaim_reg;
}

// pg 89 of c906
static void intr_completion(uint32_t target) {
    uint64_t mapbaddr = get_mapbaddr();

    // write id of intr to corresponding claim/complete reg
    uint32_t *plic_h0_mclaim = (uint32_t *)(PLIC_H0_MCLAIM + mapbaddr);
    uint32_t mclaim_reg = get32(plic_h0_mclaim);
    put32(plic_h0_mclaim, target); // shouldnt be _check? c906 pg 95
}

__attribute__((aligned(4))) void handler(void) {
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

void set_intr_prio(uint32_t irq, unsigned prio) {
    if (prio >= 32)
        // disallowed priority
        return;

    uint64_t mapbaddr = get_mapbaddr();
    uint32_t *plic_prio_base = (uint32_t *)(PLIC_PRIO_BASE + mapbaddr);
    put32_check(plic_prio_base + irq, prio);
}

void set_intr_th(unsigned th) {
    if (th >= 32)
        // disallowed threshold
        return;

    // intr only triggered if
    // prio of intr > th
    uint64_t mapbaddr = get_mapbaddr();
    uint32_t *plic_h0_mth = (uint32_t *)(PLIC_H0_MTH + mapbaddr);
    put32_check(plic_h0_mth, th);
}

uint32_t check_intr_pending(uint32_t irq) {
    uint64_t mapbaddr = get_mapbaddr();
    uint32_t *plic_ip_base = (uint32_t *)(PLIC_IP_BASE + mapbaddr);
    uint32_t ip_reg = get32(plic_ip_base + (irq >> 5));
    return (ip_reg >> (irq & 0b11111)) & 1;
}

void disable_plic_interrupt(uint32_t irq) {
    uint64_t mapbaddr = get_mapbaddr();
    uint32_t *plic_h0_mie_base = (uint32_t *)(PLIC_H0_MIE_BASE + mapbaddr);

    uint32_t ie_reg = get32(plic_h0_mie_base + (irq >> 5));
    ie_reg &= ~(1 << (irq & 0b11111));
    put32_check(plic_h0_mie_base + (irq >> 5), ie_reg);
}

void disable_all_plic_interrupts(void) {
    uint64_t mapbaddr = get_mapbaddr();
    uint32_t *plic_h0_mie_base = (uint32_t *)(PLIC_H0_MIE_BASE + mapbaddr);

    // disable all interrupts
    // load access fault beyond 0x0002008 
    for (int i=0; i<3; i++) {
        put32_check(plic_h0_mie_base + i, 0);
    }
}

void disable_ext_interrupts(void) {
    asm volatile("li t0, 0x800");
    asm volatile("csrrc t1, mie, t0");
}

void enable_ext_interrupts(void) {
    asm volatile("li t0, 0x800");
    asm volatile("csrrs t1, mie, t0");
}

void disable_interrupts(void) {
    asm volatile("csrci mstatus, 0x8");
}

void enable_interrupts(void) {
    asm volatile("csrsi mstatus, 0x8");
}

void enable_plic_interrupt(uint32_t irq) {
    uint64_t mapbaddr = get_mapbaddr();
    uint32_t *plic_h0_mie_base = (uint32_t *)(PLIC_H0_MIE_BASE + mapbaddr);

    uint32_t ie_reg = get32(plic_h0_mie_base + (irq / 32));
    ie_reg |= 1 << (irq % 32);
    put32_check(plic_h0_mie_base + (irq / 32), ie_reg);
}

void kmain(void) {
  uart_init(UART0, 115200);

  uint64_t mapbaddr = get_mapbaddr();

  // disable all interrupts (plic + clint)
  // this is a global disable
  // even the timer and sw interrupts get
  // disabled
  disable_interrupts();

  // disable external interrupts
  disable_ext_interrupts();
  
  // disable plic interrupts
  disable_all_plic_interrupts();

  // clear outstanding interrupts
  // this may be insufficient?
  // need to have interrupts enabled? bouf sdk does...
  uint32_t *plic_h0_mclaim = (uint32_t *)(PLIC_H0_MCLAIM + mapbaddr);
  uint32_t mclaim_reg = get32(plic_h0_mclaim);
  put32_check(plic_h0_mclaim, mclaim_reg);

  // setup trap+exception handler
  vector_base_set((uint32_t *)handler);

  // setup plic interrupt threshold
  set_intr_th(0);

  uart_puts(UART0, "Starting setting interrupt priorities\n");

  for (int i=4; i<=64; i++) {
      uart_puts(UART0, "Setting priority for interrupt #\n");
      itoa_hex(IRQ_NUM_BASE + i);
      set_intr_prio(IRQ_NUM_BASE + i, 1);
  }

  uart_puts(UART0, "Finished setting interrupt priorities\n");
  
  timer0_init(10000, 20000, 32000);
  timer1_init(10000, 20000, 32000);
  uint32_t ctr0 = GET32(timer0_ctr_val);
  uint32_t ctr1 = GET32(timer1_ctr_val);
  uart_puts(UART0, "Initial counter0 after timer init: ");
  itoa_hex(ctr0);
  uart_puts(UART0, "Initial counter1 after timer init: ");
  itoa_hex(ctr1);
  
  mclaim_reg = get32(plic_h0_mclaim);
  uart_puts(UART0, "Printing MCLAIM after stuff: ");
  itoa_hex(mclaim_reg);

  // enable plic interrupts
  for (int i=4; i<=64; i++) {
      uart_puts(UART0, "Enabling interrupt: ");
      itoa_hex(IRQ_NUM_BASE + i);
      enable_plic_interrupt(IRQ_NUM_BASE + i);
  }
  // uart_puts(UART0, "Enabling interrupt #61\n");
  // enable_plic_interrupt(IRQ_NUM_BASE + 61);
  // uart_puts(UART0, "Enabling interrupt #62\n");
  // enable_plic_interrupt(IRQ_NUM_BASE + 62);
  // uint32_t *plic_h0_mie_base = (uint32_t *)(PLIC_H0_MIE_BASE + mapbaddr);
  // put32_check(plic_h0_mie_base + 2, 0xffffffff);


  for (int i=4; i<=66; i++) {
      uart_puts(UART0, "Checking pending status of interrupt #");
      itoa_hex(IRQ_NUM_BASE + i);
      uint32_t pending = check_intr_pending(IRQ_NUM_BASE + i);
      itoa_hex(pending);
  }

  // enable external interrupts
  enable_ext_interrupts();

  // enable all interrupts (plic + clint)
  // this is a global enable
  enable_interrupts();

  // start the timer
  timer0_start();
  timer1_start();

  mclaim_reg = get32(plic_h0_mclaim);
  uart_puts(UART0, "Printing MCLAIM after more stuff: ");
  itoa_hex(mclaim_reg);
    
  uint32_t *plic_ip_base = (uint32_t *)(PLIC_IP_BASE + mapbaddr);
  uint32_t ip_reg = get32(plic_ip_base + ((IRQ_NUM_BASE + 61) >> 5));
  ip_reg |= 1 << ((IRQ_NUM_BASE + 61) & 0b11111);
  put32(plic_ip_base + ((IRQ_NUM_BASE + 61) >> 5), ip_reg);

  while(1) {
      // uint32_t ctr = GET32(timer1_ctr_val);
      // itoa_hex(ctr);
      // uint64_t mip = get_mip();
      // itoa_hex(mip);
      uint32_t stat0 = GET32(timer1_match_status);
      uart_puts(UART0, "stat0: ");
      itoa_hex(stat0);
      // uint32_t stat1 = GET32(timer1_match_status);
      // uart_puts(UART0, "stat1: ");
      // itoa_hex(stat1);
      // asm volatile("wfi");
      delay_ms(100);
  }
}

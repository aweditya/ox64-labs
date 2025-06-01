#define LOG_LEVEL 3
#include "lib.h"

#define DCSR        0x7b0
#define DPC         0x7b1
#define DSCRATCH0   0x7b2
#define DSCRATCH1   0x7b3

#define TSELECT     0x7a0
#define TDATA1      0x7a1
#define MCONTROL    0x7a1
#define ICOUNT      0x7a1
#define ITRIGGER    0x7a1
#define ETRIGGER    0x7a1
#define TDATA2      0x7a2
#define TDATA3      0x7a3
#define TEXTRA32    0x7a3
#define TEXTRA64    0x7a3
#define TINFO       0x7a4
#define TCONTROL    0x7a5
#define MCONTEXT    0x7a8
#define SCONTEXT    0x7aa

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

void disable_interrupts(void) {
    asm volatile("csrci mstatus, 0x8");
    return;
}

void enable_interrupts(void) {
    asm volatile("csrsi mstatus, 0x8");
    return;
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


static inline uint64_t read_csr(uint64_t csr) {
    uint64_t result;
    asm volatile (
        "csrr %0, %1"
        : "=r" (result)
        : "i" (csr)
    );
    return result;
}

static inline void write_csr(uint64_t csr, uint64_t val) {
    asm volatile (
        "csrw %0, %1"
        :
        : "i" (csr), "r" (val)
    );
}


__attribute__((aligned(4))) void handler(void) {
    uart_puts(UART0, "entering handler...\n");
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
    uart_puts(UART0, "exiting handler...\n");
}

// used to confirm that mxlen is 64 bits
static inline uint64_t get_misa(void) {
    uint64_t result;
    asm volatile("csrr %0, misa" : "=r"(result));
    return result;
}

typedef struct {
    uint64_t load       : 1;
    uint64_t store      : 1;
    uint64_t execute    : 1;
    uint64_t u          : 1;
    uint64_t s          : 1;
    uint64_t rsvd       : 1;
    uint64_t m          : 1;
    uint64_t match      : 4;
} mcontrol;

// xlen is 64 for us
// https://five-embeddev.com/riscv-priv-isa-manual/Priv-v1.12/machine.html
// we're running in m-mode so theres some action=0 (breakpoint exception)
// problem firing in m-mode trap handlers (im not worrying about that for
// now)
// so, this isnt working...
// the only thing that i havent considered is that you need virtual memory
// for addr matching...
void kmain(void) {
  volatile uint32_t x = 0xdeadbeef;
  volatile uint32_t *null = &x;
  uart_init(UART0, 115200);

  disable_interrupts();
  vector_base_set((uint64_t *)handler);
  enable_interrupts();

  put32(null, 0xdeadbeef);
  uart_puts(UART0, "Initial value at illegal address: ");
  itoa_hex(get32(null));

  // this will fail because you need to be in debug mode
  // uart_puts(UART0, "DCSR: ");
  // uint64_t dcsr = read_csr(DCSR);
  // itoa_hex(dcsr);

  // for 6 onwards, there isnt a trigger
  // by the looks of things
  // riscv ext debug support v0.13.2
  for (int i = 0; i <= 5; i++) {
      write_csr(TSELECT, i);
      uint64_t tselect = read_csr(TSELECT);
      if (tselect != i) {
          uart_puts(UART0, "Something is wrong!");
          break;
      }

      // for tselect = 0 to 3, tinfo is 0x4 => addr/data match trigger
      // for tselect = 4 to 5, tinfo is 0x38 => icount, intr, exception trigger
      uint64_t tinfo = read_csr(TINFO);
      itoa_hex(tinfo);
  }

  for (int i = 0; i <= 5; i++) {
      write_csr(TSELECT, i);

      // for tselect = 0 to 3, type is 2, dmode = 0:
      // can write tdata registers in debug and m-mode
      //
      // for tselect = 4 to 5, type is 5, dmode = 0:
      // can write tdata registers in debug and m-mode
      uint64_t tdata1 = read_csr(TDATA1);
      uart_puts(UART0, "TDATA1: ");
      itoa_hex(tdata1);
  }

#define ACTION_BKPT     0
#define ACTION_DBG      1

#define MATCH_ON_ADDR   0
#define MATCH_ON_VAL    1

  // try to get a match trigger
  // select trigger 0
  write_csr(TSELECT, 0);
  if (read_csr(TSELECT) != 0) {
      uart_puts(UART0, "There is some problem!\n");
  } else {
      uart_puts(UART0, "Wrote 0 to TSELECT!\n");
      uint64_t mcontrol = 0;
      mcontrol |= 1 << 0;               // trigger on load
      mcontrol |= 1 << 1;               // trigger on store
      mcontrol |= 0 << 2;               // no trigger on exec
      mcontrol |= 1 << 3;               // enable in u-mode
      mcontrol |= 1 << 4;               // enable in s-mode
      mcontrol |= 1 << 6;               // enable in m-mode
      mcontrol |= 0 << 7;               // match when eq to tdata2
      mcontrol |= 0 << 11;              // take action on match
      mcontrol |= ACTION_BKPT << 12;    // raise bkpt on match 
      // mcontrol |= ACTION_DBG << 12;     // go to dbg mode on match 
      
      mcontrol |= 0 << 16;              // match against any access sz
      mcontrol |= 0 << 18;              // action taken before store commits
      mcontrol |= MATCH_ON_ADDR << 19;  // match on addr
      
      // so the below doesn't look like its implemented on the board
      // saying this because the write fails
      // mcontrol |= MATCH_ON_VAL << 19;   // match on val

      if (read_csr(TSELECT) == 0)
          uart_puts(UART0, "Ok so we're looking at the right one!\n");

      // put null in tdata2
      uart_puts(UART0, "Putting null in TDATA2: ");
      itoa_hex((uint64_t)null);
      write_csr(TDATA2, (uint64_t)null);

      // uart_puts(UART0, "Putting beefbabe in TDATA2: ");
      // itoa_hex((uint64_t)0xbeefbabe);
      // write_csr(TDATA2, (uint64_t)0xbeefbabe);

      if (read_csr(TDATA2) != (uint64_t)null) {
          uart_puts(UART0, "Write for TDATA2 didn't go through?\n");
      }

      // rmw mcontrol/tdata1
      uint64_t old = read_csr(MCONTROL);
      uart_puts(UART0, "Original MCONTROL: ");
      itoa_hex(old);

      uint64_t new = old | mcontrol;
      write_csr(MCONTROL, new);
      if (read_csr(MCONTROL) != new) {
          uart_puts(UART0, "Write for MCONTROL didn't go through?\n");
      }

      uart_puts(UART0, "New MCONTROL: ");
      itoa_hex(new);

      // expect a breakpoint exception here
      put32(null, 0xbeefbabe);

      // so this is really weird...
      // since dmode is not enabled for this trigger, we cant go to
      // debug mode on a match so dcsr is not accessible. but, to
      // be able to modify dmode, you need to be in debug mode so...
      // uart_puts(UART0, "DCSR: ");
      // uint64_t dcsr = read_csr(DCSR);
      // itoa_hex(dcsr);

      uart_puts(UART0, "Printing x after mod: ");
      itoa_hex(x);
  }

  // try to get a single-step trigger
  write_csr(TSELECT, 4);
  if (read_csr(TSELECT) != 4) {
      uart_puts(UART0, "There is some problem!\n");
  } else {
      uart_puts(UART0, "Wrote 4 to TSELECT\n");

      // we're actually getting exception triggers
      // going to enable load access fault exceptions
      // also going to enable ecall from m-mode
      uint64_t tdata2 = read_csr(TDATA2);
      uart_puts(UART0, "Original TDATA2: ");
      itoa_hex(tdata2);

      write_csr(TDATA2, 1<<4 | 1<<5 | 1<<11);
      tdata2 = read_csr(TDATA2);
      if (tdata2 != (1<<4 | 1<<5 | 1<<11)) {
          uart_puts(UART0, "Load access fault bpkt not supported! Got: ");
          itoa_hex(tdata2);
      }

      uint64_t etrigger = read_csr(ETRIGGER);
      uart_puts(UART0, "Old ETRIGGER: ");
      itoa_hex(etrigger);

      uint64_t new = etrigger | 1<<6 | 1<<7 | 1<<9;
      uart_puts(UART0, "New ETRIGGER: ");
      itoa_hex(new);
      write_csr(ETRIGGER, new);
       
      // ideally, doing a load from an illegal addr
      // should throw a bkpt exception
      // GET32(0x0);
  }

  write_csr(TSELECT, 5);
  if (read_csr(TSELECT) != 5) {
      uart_puts(UART0, "There is some problem!\n");
  } else {
      uart_puts(UART0, "Wrote 5 to TSELECT\n");

      // write 0 to tdata1 (warl: write-any-legal-value)
      write_csr(TDATA1, 0x0);

      uint64_t tdata1 = read_csr(TDATA1);
      uart_puts(UART0, "Original TDATA1: ");
      itoa_hex(tdata1);

      tdata1 &= 0x0fffffffffffffff;
      tdata1 |= 3LL << 60;
      
      write_csr(TDATA1, tdata1);

      uint64_t tdata1_ = read_csr(TDATA1);
      if ((tdata1_ >> 60) != 3) {
          uart_puts(UART0, "TDATA type didn't change! Got: ");
          itoa_hex(tdata1_ >> 60);
      }

      // type 3 so tdata1 is icount
      uart_puts(UART0, "ICOUNT: ");
      itoa_hex(tdata1_);

      tdata1_ |= 1<<6;  // enable single step in u-mode
      tdata1_ |= 1<<7;  // enable single step in s-mode
      tdata1_ |= 1<<9;  // enable single step in m-mode

      write_csr(ICOUNT, tdata1_);
      if (read_csr(ICOUNT) != tdata1_) {
          uart_puts(UART0, "Write to ICOUNT failed!\n");
      }
  }

  uint64_t tdata1 = read_csr(TDATA1);
  while(1) {
      itoa_hex(tdata1);
      delay_ms(1000);
      // asm volatile("wfi");
  }
}

/*
 * CLINT driver. Interrupt handling on the Ox64
*/
#define LOG_LEVEL 3
#include "lib.h"

volatile uint32_t *const MSIP0 = (volatile uint32_t *)0xe4000000;
volatile uint32_t *const MTIMECMPL0 = (volatile uint32_t *)0xe4004000;
volatile uint32_t *const MTIMECMPH0 = (volatile uint32_t *)0xe4004004;

// Helper function to convert an integer to a hexadecimal string and print using putc
void itoa_hex(uint64_t num);

void disable_interrupts(void);

void enable_interrupts(void);

void disable_timer_interrupts(void);

void enable_timer_interrupts(void);

void disable_sw_interrupts(void);

void enable_sw_interrupts(void);

void gen_sw_interrupts(void);

void my_timer_init(uint32_t mtime_cmp_lo, uint32_t mtime_cmp_hi);

inline uint64_t get_mtvec(void);

inline uint64_t get_mstatus(void);

inline uint64_t get_mie(void);

inline uint64_t get_satp(void);

inline uint64_t get_mip(void);

inline uint64_t get_mxstatus(void);

inline void vector_base_set(void *vec);

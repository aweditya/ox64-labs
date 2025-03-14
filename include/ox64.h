#pragma once
#include <stdint.h>

extern void put32(volatile uint32_t *addr, uint32_t value);
extern void PUT32(volatile uint32_t addr, uint32_t value);

extern void put64(volatile uint64_t *addr, uint32_t value);
extern void PUT64(volatile uint64_t addr, uint32_t value);

extern uint32_t get32(volatile uint32_t *addr);
extern uint32_t GET32(volatile uint32_t addr);

#define get32_type(T, addr) \
  ({ \
    _Static_assert(sizeof(T) == 4); \
    uint32_t raw = get32((volatile uint32_t *)addr); \
    *(T *)&raw; \
  })
#define put32_type(T, addr, val) \
  ({ \
    _Static_assert(sizeof(T) == 4); \
    uint32_t raw = *(uint32_t *)&val; \
    put32((volatile uint32_t *)addr, raw); \
  })


/*********************************************************
 * some gcc helpers.
 */

// gcc memory barrier.
#define gcc_mb() asm volatile ("" : : : "memory")

// from linux --- can help gcc make better code layout
// decisions.  can sometimes help when we want nanosec
// accurate code.
//
// however: leave these til the last thing you do.
//
// example use:
//   if(unlikely(!(p = kmalloc(4))))
//      panic("kmalloc failed\n");
#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)


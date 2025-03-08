#pragma once
#include <stdint.h>

/**
 * Used to write to device memory.  Use this instead of a raw store.
 * Writes the 32-bit value <v> to address <addr>.
 *
 * Safer alternative to *(uint32_t *)addr = v;
 *
 * Like PUT32, but takes <addr> as a pointer.
 */
void put32(volatile void *addr, uint32_t v);

/**
 * Used to write to device memory.  Use this instead of a raw store.
 * Writes the 32-bit value <v> to address <addr>.
 *
 * Safer alternative to *(uint32_t *)addr = v;
 *
 * Like put32, but takes <addr> as an int.
 */
void PUT32(uint32_t addr, uint32_t v);

/**
 * Used to read from device memory.  Use this instead of a raw
 * dereference. Returns the 32-bit value at address <addr>.
 *
 * Safer alternative to *(uint32_t *)addr.
 *
 * Like GET32, but takes <addr> as a pointer.
 */
uint32_t get32(const volatile void *addr);

/**
 * Used to read from device memory.  Use this instead of a
 * raw dereference. Returns the 32-bit value at address <addr>.
 *
 * Safer alternative to *(uint32_t *)addr.
 *
 * Like get32, but takes <addr> as an int.
 */
uint32_t GET32(uint32_t addr);

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


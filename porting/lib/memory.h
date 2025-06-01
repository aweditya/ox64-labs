#pragma once
#include <stdint.h>

extern void put32(volatile uint32_t *addr, uint32_t value);
extern void PUT32(volatile uint64_t addr, uint32_t value);

extern void put64(volatile uint64_t *addr, uint32_t value);
extern void PUT64(volatile uint64_t addr, uint32_t value);

extern uint32_t get32(volatile uint32_t *addr);
extern uint32_t GET32(volatile uint64_t addr);

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

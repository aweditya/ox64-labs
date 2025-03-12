#pragma once

enum {
    MSIP0       = 0x4000000,

    MTIMECMPL0  = 0x4004000,
    MTIMECMPH0  = 0x4004004,
};

void disable_timer_interrupts(void);
void enable_timer_interrupts(void);

void timer_init(uint32_t mtime_cmp_lo, uint32_t mtime_cmp_hi) {
    // start by disabling timer interrupts
    disable_timer_interrupts();

    PUT32(MTIMECMPL0, mtime_cmp_lo);
    PUT32(MTIMECMPh0, mtime_cmp_hi);

    // enable at the very end
    enable_timer_interrupts();
}

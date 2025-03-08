#pragma once

enum {
    timer_clk_src           = 0x2000a500,

    timer2_match_val0       = 0x2000a510,
    timer2_match_val1       = 0x2000a514,
    timer2_match_val2       = 0x2000a518,

    timer2_ctr_val2         = 0x2000a52c,

    timer2_match_status     = 0x2000a538,
    timer2_match_intr_en    = 0x2000a544,

    timer2_preload_val      = 0x2000a550,
    timer2_preload_ctl      = 0x2000a55c,

    timer2_intr_clr         = 0x2000a578,
    timer_ctr_en_clr        = 0x2000a584,

    timer2_match_intr_mode  = 0x2000a590,
    timer_clk_div           = 0x2000a5bc,
};

enum {
    SRC_FCLK    = 0,
    SRC_32K     = 1,
    SRC_1K      = 2,
    SRC_BCLK    = 3,
    SRC_GPIO    = 4,
    SRC_NOCLK   = 5,
};

void timer2_init() {
    // set clk src (use 32K for now)
    PUT32

}


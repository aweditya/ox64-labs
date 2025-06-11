#define LOG_LEVEL 3
#include "lib.h"

static volatile uint32_t *const gpio_cfg0 = (volatile uint32_t *)0x200008c4;

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
}

#define UART_CLOCK  40000000UL
#define BAUD        9600
#define BAUD_TO_CYCLES(baud) ((UART_CLOCK)/baud)

#define VAL_N_US(us)   (((us) * FREQ) / 1000000)

#define ON(pin)  do { \
    uint32_t val = get32(gpio_cfg0 + (pin)); \
    val |= (1 << 25); \
    put32(gpio_cfg0 + (pin), val); \
} while (0)

typedef struct {
    unsigned rx;
    unsigned tx;
    unsigned cyc_per_bit;
} sw_uart_t;

static void sw_uart_init(sw_uart_t *s, unsigned rx, unsigned tx) {
    // default baud rate is 115200
    s->rx = rx;
    s->tx = tx;
    s->cyc_per_bit = BAUD_TO_CYCLES(BAUD);

    gpio_set_input(rx);
    gpio_set_output(tx);
}

static inline void gpio_write_on_raw(unsigned pin) {
    uint32_t val = get32(gpio_cfg0 + (pin));
    val |= (1 << 25);
    put32(gpio_cfg0 + (pin), val);
}

static inline void gpio_write_off_raw(unsigned pin) {
    uint32_t val = get32(gpio_cfg0 + (pin));
    val |= (1 << 26);
    put32(gpio_cfg0 + (pin), val);
}

static inline void wait_for(unsigned s, unsigned delay) {
    while (cycle_cnt_read() - s <= delay)
        ;
}

void sw_uart_put8(sw_uart_t *s, uint8_t b) {
    unsigned start;
    unsigned bit_width = s->cyc_per_bit;
    start = cycle_cnt_read();

    gpio_write_off_raw(s->tx);
    wait_for(start, bit_width);

    for (unsigned i = 0; i < 8; i++) {
        if (b & (1 << i))
            gpio_write_on_raw(s->tx);
        else
            gpio_write_off_raw(s->tx);

    
        wait_for(start, (i+2)*bit_width);
    }

    gpio_write_on_raw(s->tx);
    wait_for(start, 10*bit_width);
}

enum { tx=16, rx=28 };

void kmain(void) {
    sw_uart_t s;
    sw_uart_init(&s, rx, tx);

    uart_init(UART0, 115200);

    unsigned s1 = cycle_cnt_read();
        gpio_write_on_raw(tx);
    unsigned s2 = cycle_cnt_read();
    
    while (1) {
        itoa_hex(s2-s1); uart_puts(UART0, "\r\n");
        itoa_hex(s.cyc_per_bit); uart_puts(UART0, "\r\n");
        delay_ms(1000);
    }
}


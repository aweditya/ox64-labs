#define LOG_LEVEL 3
#include "lib.h"

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

#define FREQ        (38 * 1000)
#define DELAY_US    ((1000 * 1000) / (FREQ * 2))

#define VAL_N_US(us)   (((us) * FREQ) / 1000000)

#define ON(pin)  do { \
    uint32_t val = get32(gpio_cfg0 + (pin)); \
    val |= (1 << 25); \
    put32(gpio_cfg0 + (pin), val); \
} while (0)

#define OFF(pin) do { \
    uint32_t val = get32(gpio_cfg0 + (pin)); \
    val |= (1 << 26); \
    put32(gpio_cfg0 + (pin), val); \
} while (0)

#define GENERATE_WAVEFORM(pin, duration_us) do { \
    for (int _i = 0; _i < VAL_N_US(duration_us); _i++) { \
        ON(pin); \
        delay_us(DELAY_US); \
        OFF(pin); \
        delay_us(DELAY_US); \
    } \
} while (0)

#define SEND_HEADER(pin) do { \
    GENERATE_WAVEFORM(pin, 9000); \
    delay_us(4500); \
} while (0)

#define SEND_ZERO(pin) do { \
    GENERATE_WAVEFORM(pin, 600); \
    delay_us(600); \
} while (0)

#define SEND_ONE(pin) do { \
    GENERATE_WAVEFORM(pin, 600); \
    delay_us(1600); \
} while (0)

#define SEND_BIT(pin, bit) do { \
    if ((bit) == 0) { \
        SEND_ZERO(pin); \
    } else { \
        SEND_ONE(pin); \
    } \
} while (0)

#define SEND_STOP(pin) do { \
    GENERATE_WAVEFORM(pin, 600); \
    delay_us(40000); \
} while (0)

#define SEND_BYTE(pin, byte) do { \
    SEND_BIT(pin, (byte) & 0x80); \
    SEND_BIT(pin, (byte) & 0x40); \
    SEND_BIT(pin, (byte) & 0x20); \
    SEND_BIT(pin, (byte) & 0x10); \
    SEND_BIT(pin, (byte) & 0x08); \
    SEND_BIT(pin, (byte) & 0x04); \
    SEND_BIT(pin, (byte) & 0x02); \
    SEND_BIT(pin, (byte) & 0x01); \
} while (0)

static volatile uint32_t *const gpio_cfg0 = (volatile uint32_t *)0x200008c4;

void kmain(void) {
    uart_init(UART0, 115200);
    gpio_set_output(17);

    while (1) {
        SEND_HEADER(17);
        
        SEND_BYTE(17, 0x48);

        SEND_STOP(17);

        delay_ms(1000);
    }
}


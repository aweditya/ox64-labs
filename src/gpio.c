#include "gpio.h"

void gpio_set_function(unsigned pin, gpio_func_t function) {
    gpio_cfg_t cfg = get32_type(gpio_cfg_t, GPIO_CFG0 + pin);
    cfg.func_sel = function;
    put32_type(gpio_cfg_t, GPIO_CFG0 + pin, cfg);
}

void gpio_set_input(unsigned pin) {
    gpio_cfg_t cfg = get32_type(gpio_cfg_t, GPIO_CFG0 + pin);
    cfg.input_enable = 1;
    cfg.func_sel = GPIO_FUNC_GPIO;
    put32_type(gpio_cfg_t, GPIO_CFG0 + pin, cfg);
}

// add an argument that accepts gpio_op_mode_t
void gpio_set_output(unsigned pin) {
    gpio_cfg_t cfg = get32_type(gpio_cfg_t, GPIO_CFG0 + pin);
    cfg.output_enable = 1;
    cfg.func_sel = GPIO_FUNC_GPIO;
    // cfg.drv_control = 3;

    // using set/clr mode for the time being
    cfg.mode = 0b01;

    put32_type(gpio_cfg_t, GPIO_CFG0 + pin, cfg);
}

void gpio_write(unsigned pin, unsigned val) {
    gpio_cfg_t cfg = get32_type(gpio_cfg_t, GPIO_CFG0 + pin);

    // should probably add some logic that
    // checks for gpio_op_mode_t
    if (val == 0)
        cfg.clr = 1;
    else
        cfg.set = 1;
    put32_type(gpio_cfg_t, GPIO_CFG0 + pin, cfg);
}

int gpio_read(unsigned pin) {
    gpio_cfg_t cfg = get32_type(gpio_cfg_t, GPIO_CFG0 + pin);

    // make sure we are in input mode
    return cfg.input_val;
}

void gpio_set_on(unsigned pin) {
    gpio_cfg_t cfg = get32_type(gpio_cfg_t, GPIO_CFG0 + pin);

    // should probably add some logic that
    // checks for gpio_op_mode_t
    cfg.set = 1;
    cfg.clr = 0;
    put32_type(gpio_cfg_t, GPIO_CFG0 + pin, cfg);
}

void gpio_set_off(unsigned pin) {
    gpio_cfg_t cfg = get32_type(gpio_cfg_t, GPIO_CFG0 + pin);

    // should probably add some logic that
    // checks for gpio_op_mode_t
    cfg.set = 0;
    cfg.clr = 1;
    put32_type(gpio_cfg_t, GPIO_CFG0 + pin, cfg);
}

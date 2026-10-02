#include "rpi.h"
#include <stdint.h>

#define OUTPUT 0b001
#define INPUT 0b000

#include "asm-helpers.h"

cp_asm_raw(cp15_scratch2, p15, 0, c13, c0, 3)
cp_asm_raw(cp15_scratch1, p15, 0, c13, c0, 2)


enum {
    // Max gpio pin number.
    GPIO_MAX_PIN = 53,

    GPIO_BASE = 0x20200000,
    gpio_set0 = (GPIO_BASE + 0x1C),
    gpio_clr0 = (GPIO_BASE + 0x28),
    gpio_lev0 = (GPIO_BASE + 0x34),
    gpio_eds0 = (GPIO_BASE + 0x40),

    // <you will need other values from BCM2835!>
};

static inline void raw_gpio_set_mode(unsigned pin, unsigned mode) {
    volatile unsigned *addr = (volatile unsigned *)(GPIO_BASE + ((pin / 10) << 2));
    unsigned val = *addr;
    /* unsigned val = get32(addr); */
    unsigned mask = 0b111 << (pin % 10 * 3);
    val &= ~mask;
    val |= mode << (pin % 10 * 3);
    /* put32(addr, val); */
    *addr = val;
}

//
// Part 1 implement gpio_set_on, gpio_set_off, gpio_set_output
//

// set <pin> to be an output pin.
//
// NOTE: fsel0, fsel1, fsel2 are contiguous in memory, so you
// can (and should) use ptr calculations versus if-statements!
static inline void raw_gpio_set_output(unsigned pin) {
    raw_gpio_set_mode(pin, OUTPUT);
}

// Set GPIO <pin> = on.
static inline void raw_gpio_set_on(unsigned pin) {
    volatile unsigned *addr = (volatile unsigned *)(gpio_set0 + ((pin > 31) << 2));
    unsigned val = 0b1 << (pin & 31);
    *addr = val;
    /* put32(addr, val); */

}

// Set GPIO <pin> = off
static inline void raw_gpio_set_off(unsigned pin) {
    volatile unsigned *addr = (volatile unsigned *)(gpio_clr0 + ((pin > 31) << 2));
    unsigned val = 0b1 << (pin & 31);
    /* put32(addr, val); */
    *addr = val;
}

static inline void raw_gpio_event_clear(unsigned pin) {
    // raw_gpio_set_off(pin);
    /* volatile unsigned *addr = (volatile unsigned*)(gpio_eds0 + ((pin > 31) << 2)); */
    volatile unsigned *addr = (volatile unsigned*)(cp15_scratch2_get() + ((pin > 31) << 2));
    unsigned val = 0b1 << (pin & 31);
    *addr = val;
}

// Set <pin> to <v> (v \in {0,1})
static inline void raw_gpio_write(unsigned pin, unsigned v) {
    if (v)
        raw_gpio_set_on(pin);
    else
        raw_gpio_set_off(pin);
}

//
// Part 2: implement gpio_set_input and gpio_read
//

// set <pin> = input.
static inline void raw_gpio_set_input(unsigned pin) {
    raw_gpio_set_mode(pin, INPUT);
}

// Return 1 if <pin> is on, 0 if not.
static inline int raw_gpio_read(unsigned pin) {
    unsigned v = 0;

    /* v = get32((unsigned *)(gpio_lev0 + ((pin > 31) << 2))); */
    v = *((volatile unsigned*)gpio_lev0 + ((pin > 31) << 2));
    v &= (0b1 << (pin & 31));
    v >>= pin;
    return v;
}

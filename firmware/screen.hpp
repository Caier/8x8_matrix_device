#pragma once

#include <stdint.h>
#include <avr/io.h>

#define DDR(x) (*(&x - 1))

struct Pin {
    volatile uint8_t& port;
    int pin;

    inline void set_pin(bool val) const {
        val ? (port |= (1 << pin)) : (port &= ~(1 << pin)); 
    }
};

static const Pin row[] = {
    PORTD, 0,
    PORTD, 4,
    PORTC, 5,
    PORTD, 1,
    PORTB, 4,
    PORTC, 4,
    PORTB, 3,
    PORTC, 2,
};

static const Pin col[] = {
    PORTB, 1,
    PORTB, 0,
    PORTD, 6,
    PORTC, 3,
    PORTD, 5,
    PORTC, 1,
    PORTC, 0,
    PORTD, 7,
};

struct Screen {
    uint64_t state = 0x0FF00FF00FF00FF0;
    bool enabled = true;

    Screen();
    void render();
};
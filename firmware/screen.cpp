#include "screen.hpp"

extern "C" {
    #include <avr/delay.h>
    #include "usbdrv/usbdrv.h"
}

Screen::Screen() {
    for(const auto& p : row) {
        DDR(p.port) |= (1 << p.pin);
        p.port &= ~(1 << p.pin);
    }

    for(const auto& p : col) {
        DDR(p.port) |= (1 << p.pin);
        p.port |= (1 << p.pin);
    }
}

__attribute__((always_inline)) void Screen::render() {
    auto state = this->state;

    if(!enabled) {
        usbPoll();
        return;
    }

    for(char c = 0; c < 8; ++c) {
        col[c].set_pin(0);
        for(char r = 0; r < 8; ++r) {
            row[r].set_pin((state & (1ULL << 63)) >> 63);
            state <<= 1;
        }
        usbPoll();
        //_delay_us(10);
        for(char r = 0; r < 8; ++r)
            row[r].set_pin(0);
        col[c].set_pin(1);
    }
}
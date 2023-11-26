#include <avr/io.h>
#include <util/delay.h>
#include <avr/sleep.h>

struct Pin {
    volatile uint8_t& port;
    int pin;
};

const Pin row[] = {
    PORTC, 0,
    PORTB, 3,
    PORTC, 2,
    PORTB, 2,
    PORTD, 5,
    PORTC, 3,
    PORTD, 7,
    PORTD, 2,
};

const Pin col[] = {
    PORTB, 1,
    PORTB, 0,
    PORTD, 3,
    PORTC, 1,
    PORTD, 4,
    PORTB, 5,
    PORTB, 4,
    PORTD, 6,
};

inline void set_pin(Pin p, bool val) {
    val ? (p.port |= (1 << p.pin)) : (p.port &= ~(1 << p.pin)); 
}

//TODO: don't use arduino bootloader, use arduino as isp

int main() {
    DDRB = 0xff;
    DDRC = 0xff;
    DDRD = 0xff;

    for(int i = 0; i < 8; i++) {
        set_pin(col[i], 0);
        set_pin(row[i], 1);
    }

    _delay_ms(5000);

    for(auto p : row)
        set_pin(p, 0);

    sleep_mode();
}
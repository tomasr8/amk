#ifndef F_CPU
#define F_CPU 16000000UL  // 16mhz
#endif

#include <avr/io.h>
#include <stdbool.h>
#include <stdint.h>
#include <util/delay.h>
#include <util/atomic.h>

#include "keys.h"

#define B (0 << 6)
#define C (1 << 6)
#define D (2 << 6)
#define E (3 << 6)

#define KPORTB2 (B | PORTB2)
#define KPORTB3 (B | PORTB3)
#define KPORTB4 (B | PORTB4)
#define KPORTB5 (B | PORTB5)
#define KPORTB6 (B | PORTB6)

#define KPORTE6 (E | PORTE6)

#define NUM_ROWS 2
#define NUM_COLS 4

typedef struct {
    uint8_t modifiers;
    bool is_overflow;
    uint8_t pressed_keys[NUM_ROWS * NUM_COLS];
} keyboard_state_t;

void init_pins();
bool matrix_scan();
keyboard_state_t get_state();

__attribute__((always_inline)) static inline bool is_modifier_key(
    uint8_t keycode) {
    return (keycode >= 0xe0) && (keycode <= 0xe7);
}

__attribute__((always_inline)) static inline void set_as_output(uint8_t pin) {
    switch (pin & (3 << 6)) {
        case B:
            DDRB |= (1 << (pin & 0x3f));
            break;
        case C:
            DDRC |= (1 << (pin & 0x3f));
            break;
        case D:
            DDRD |= (1 << (pin & 0x3f));
            break;
        case E:
            DDRE |= (1 << (pin & 0x3f));
            break;
    }
}

__attribute__((always_inline)) static inline void set_as_input(uint8_t pin) {
    switch (pin & (3 << 6)) {
        case B:
            DDRB &= ~(1 << (pin & 0x3f));
            break;
        case C:
            DDRC &= ~(1 << (pin & 0x3f));
            break;
        case D:
            DDRD &= ~(1 << (pin & 0x3f));
            break;
        case E:
            DDRE &= ~(1 << (pin & 0x3f));
            break;
    }
}

__attribute__((always_inline)) static inline void set_high(uint8_t pin) {
    switch (pin & (3 << 6)) {
        case B:
            PORTB |= (1 << (pin & 0x3f));
            break;
        case C:
            PORTC |= (1 << (pin & 0x3f));
            break;
        case D:
            PORTD |= (1 << (pin & 0x3f));
            break;
        case E:
            PORTE |= (1 << (pin & 0x3f));
            break;
    }
}

__attribute__((always_inline)) static inline void set_low(uint8_t pin) {
    switch (pin & (3 << 6)) {
        case B:
            PORTB &= ~(1 << (pin & 0x3f));
            break;
        case C:
            PORTC &= ~(1 << (pin & 0x3f));
            break;
        case D:
            PORTD &= ~(1 << (pin & 0x3f));
            break;
        case E:
            PORTE &= ~(1 << (pin & 0x3f));
            break;
    }
}

__attribute__((always_inline, warn_unused_result)) static inline bool read_pin(
    uint8_t pin) {
    switch (pin & (3 << 6)) {
        case B:
            return (PINB & (1 << (pin & 0x3f))) ? true : false;
        case C:
            return (PINC & (1 << (pin & 0x3f))) ? true : false;
        case D:
            return (PIND & (1 << (pin & 0x3f))) ? true : false;
        case E:
            return (PINE & (1 << (pin & 0x3f))) ? true : false;
        default:
            return false;
    }
}
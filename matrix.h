#include <avr/io.h>
#include <stdbool.h>
#include <stdint.h>
#include <util/delay.h>
#include "keys.h"

__attribute__((always_inline)) static inline bool is_modifier_key(
    uint8_t keycode) {
    return (keycode >= 0xe0) && (keycode <= 0xe7);
}

__attribute__((always_inline)) static inline void set_as_output(uint8_t pin) {
    DDRB |= (1 << pin);
}

__attribute__((always_inline)) static inline void set_as_input(uint8_t pin) {
    DDRB &= ~(1 << pin);
}

__attribute__((always_inline)) static inline void set_high(uint8_t pin) {
    PORTB |= (1 << pin);
}

__attribute__((always_inline)) static inline void set_low(uint8_t pin) {
    PORTB &= ~(1 << pin);
}

__attribute__((always_inline, warn_unused_result)) static inline bool read_pin(
    uint8_t pin) {
    return (PINB & (1 << pin)) ? true : false;
}
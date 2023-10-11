#include "matrix.h"

uint8_t keyboard_modifiers = 0;
uint8_t keyboard_keys[] = {0, 0, 0, 0, 0, 0, 0, 0};

#define NUM_ROWS 2
#define NUM_COLS 4
uint8_t col_pins[] = {PORTB0, PORTB1, PORTB2, PORTB3};
uint8_t row_pins[] = {PORTB4, PORTB5};
uint8_t keyboard_layout[NUM_ROWS][NUM_COLS] = {{0, 0, 0, 0}, {0, 0, 0, 0}};

bool keyboard_state_raw[NUM_ROWS][NUM_COLS] = {{0, 0, 0, 0}, {0, 0, 0, 0}};
bool keyboard_state[NUM_ROWS][NUM_COLS] = {{0, 0, 0, 0}, {0, 0, 0, 0}};

void init_pins() {
    for (uint8_t i = 0; i < NUM_COLS; i++) {
        set_as_output(col_pins[i]);
        set_low(col_pins[i]);
    }

    for (uint8_t i = 0; i < NUM_ROWS; i++) {
        set_as_input(row_pins[i]);
    }
}

bool matrix_scan() {
    bool changed = false;
    for (uint8_t i = 0; i < NUM_COLS; i++) {
        set_high(col_pins[i]);
        _delay_us(20);

        for (uint8_t j = 0; j < NUM_ROWS; j++) {
            const bool previous = keyboard_state_raw[j][i];
            const bool current = read_pin(row_pins[j]);
            keyboard_state_raw[j][i] = current;
            if (previous != current) {
                changed = true;
            }
        }

        set_low(col_pins[i]);
    }
    return changed;
}

void debounce() {

}

void _matrix_scan() {
    const bool changed = matrix_scan();
    debounce();

    uint8_t k = 0;
    for (uint8_t i = 0; i < NUM_COLS; i++) {
        for (uint8_t j = 0; j < NUM_ROWS; j++) {
            const uint8_t keycode = keyboard_layout[j][i];
            const bool is_pressed = read_pin(row_pins[j]);
            if (!is_pressed) {
                if (is_modifier_key(keycode)) {
                    keyboard_modifiers &= ~(1 << (keycode & ~0xe0));
                } else {
                    keyboard_keys[k] = 0;
                }
            } else {
                if (is_modifier_key(keycode)) {
                    keyboard_modifiers |= (1 << (keycode & ~0xe0));
                } else {
                    keyboard_keys[k] = keyboard_layout[j][i];
                }
            }
            k++;
        }
    }
}
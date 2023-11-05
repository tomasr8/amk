#include "matrix.h"

#define NUM_ROWS 2
#define NUM_COLS 4

typedef struct {
    uint8_t modifiers;
    bool is_overflow;
    uint8_t pressed_keys[NUM_ROWS * NUM_COLS];
} keyboard_state_t;

uint8_t keyboard_modifiers = 0;
uint8_t keyboard_keys[] = {0, 0, 0, 0, 0, 0, 0, 0};

uint8_t col_pins[] = {PORTB0, PORTB1, PORTB2, PORTB3};
uint8_t row_pins[] = {PORTB4, PORTB5};
uint8_t keyboard_layout[NUM_ROWS][NUM_COLS] = {{0, 0, 0, 0}, {0, 0, 0, 0}};

bool keyboard_state_raw[NUM_ROWS][NUM_COLS] = {{0, 0, 0, 0}, {0, 0, 0, 0}};
bool keyboard_state[NUM_ROWS][NUM_COLS] = {{0, 0, 0, 0}, {0, 0, 0, 0}};
uint8_t keyboard_pressed_keys[8] = {0, 0, 0, 0, 0, 0, 0, 0};
// uint8_t keyboard_report[6+8] = {0, 0, 0, 0, 0, 0, 0, 0};

keyboard_state_t _keyboard_state = {.modifiers = 0,
                                    .is_overflow = false,
                                    .pressed_keys = {0, 0, 0, 0, 0, 0, 0, 0}};

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

void debounce() {}

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

void reset_state() {
    _keyboard_state.modifiers = 0;
    _keyboard_state.is_overflow = false;

    for (uint8_t i = 0; i < (NUM_ROWS * NUM_COLS); i++) {
        _keyboard_state.pressed_keys[i] = 0;
    }
}

keyboard_state_t* get_pressed_keys() {
    reset_state();

    uint8_t k = 0;
    uint8_t num_pressed_keys = 0;
    for (uint8_t i = 0; i < NUM_COLS; i++) {
        for (uint8_t j = 0; j < NUM_ROWS; j++) {
            const uint8_t keycode = keyboard_layout[j][i];
            const bool is_pressed = keyboard_state[j][i];
            if (!is_pressed) {
                if (is_modifier_key(keycode)) {
                    _keyboard_state.modifiers &= ~(1 << (keycode & ~0xe0));
                } else {
                    _keyboard_state.pressed_keys[k] = 0;
                }
            } else {
                if (is_modifier_key(keycode)) {
                    _keyboard_state.modifiers |= (1 << (keycode & ~0xe0));
                } else {
                    num_pressed_keys++;
                    _keyboard_state.pressed_keys[num_pressed_keys] =
                        keyboard_layout[j][i];
                }
            }
            k++;
        }
    }

    if (num_pressed_keys > 6) {
        _keyboard_state.is_overflow = true;
    }

    return &_keyboard_state;
}
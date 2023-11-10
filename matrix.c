#include "matrix.h"

uint8_t col_pins[] = {KPORTE6, KPORTB4, KPORTB6, KPORTB2};
uint8_t row_pins[] = {KPORTB5, KPORTB3};
uint8_t keyboard_layout[NUM_ROWS][NUM_COLS] = {{KEY_A, KEY_B, KEY_C, KEY_D},
                                               {KEY_E, KEY_F, KEY_G, KEY_H}};

bool keyboard_state[NUM_ROWS][NUM_COLS] = {{0, 0, 0, 0}, {0, 0, 0, 0}};

keyboard_state_t _keyboard_state = {
    .modifiers = 0, .is_overflow = false, .pressed_keys = {0, 0, 0, 0, 0, 0}};

void init_pins() {
    for (uint8_t i = 0; i < NUM_COLS; i++) {
        ATOMIC_BLOCK(ATOMIC_FORCEON) {
            set_as_input(col_pins[i]);
            set_high(col_pins[i]);
        }
    }

    for (uint8_t i = 0; i < NUM_ROWS; i++) {
        ATOMIC_BLOCK(ATOMIC_FORCEON) {
            set_as_input(row_pins[i]);
            set_high(row_pins[i]);
        }
    }
}

bool matrix_scan() {
    bool changed = false;
    for (uint8_t j = 0; j < NUM_ROWS; j++) {
        ATOMIC_BLOCK(ATOMIC_FORCEON) {
            set_as_output(row_pins[j]);
            set_low(row_pins[j]);
        }
        _delay_ms(10);

        for (uint8_t i = 0; i < NUM_COLS; i++) {
            const bool previous = keyboard_state[j][i];
            const bool current = !read_pin(col_pins[i]);
            if (previous != current) {
                changed = true;
            }
            keyboard_state[j][i] = current;
        }

        ATOMIC_BLOCK(ATOMIC_FORCEON) {
            set_as_input(row_pins[j]);
            set_high(row_pins[j]);
        }
        _delay_ms(10);
    }
    return changed;
}

keyboard_state_t get_state() {
    _keyboard_state.modifiers = 0;
    _keyboard_state.is_overflow = false;
    for (uint8_t i = 0; i < 6; i++) {
        _keyboard_state.pressed_keys[i] = 0;
    }

    uint8_t pressed_keys = 0;
    for (uint8_t i = 0; i < NUM_COLS; i++) {
        for (uint8_t j = 0; j < NUM_ROWS; j++) {
            uint8_t key = keyboard_layout[j][i];
            bool is_pressed = keyboard_state[j][i];
            if (!is_pressed) {
                continue;
            }

            if (is_modifier_key(key)) {
                _keyboard_state.modifiers |= (1 << (key & ~0xe0));
            } else {
                if (pressed_keys >= 6) {
                    _keyboard_state.is_overflow = true;
                    return _keyboard_state;
                }
                _keyboard_state.pressed_keys[pressed_keys] =
                    keyboard_layout[j][i];
                pressed_keys++;
            }
        }
    }
    return _keyboard_state;
}

// void _matrix_scan() {
//     const bool changed = matrix_scan();
//     debounce();

//     uint8_t k = 0;
//     for (uint8_t i = 0; i < NUM_COLS; i++) {
//         for (uint8_t j = 0; j < NUM_ROWS; j++) {
//             const uint8_t keycode = keyboard_layout[j][i];
//             const bool is_pressed = read_pin(row_pins[j]);
//             if (!is_pressed) {
//                 if (is_modifier_key(keycode)) {
//                     keyboard_modifiers &= ~(1 << (keycode & ~0xe0));
//                 } else {
//                     keyboard_keys[k] = 0;
//                 }
//             } else {
//                 if (is_modifier_key(keycode)) {
//                     keyboard_modifiers |= (1 << (keycode & ~0xe0));
//                 } else {
//                     keyboard_keys[k] = keyboard_layout[j][i];
//                 }
//             }
//             k++;
//         }
//     }
// }

// void reset_state() {
//     _keyboard_state.modifiers = 0;
//     _keyboard_state.is_overflow = false;

//     for (uint8_t i = 0; i < (NUM_ROWS * NUM_COLS); i++) {
//         _keyboard_state.pressed_keys[i] = 0;
//     }
// }

// keyboard_state_t* get_pressed_keys() {
//     reset_state();

//     uint8_t k = 0;
//     uint8_t num_pressed_keys = 0;
//     for (uint8_t i = 0; i < NUM_COLS; i++) {
//         for (uint8_t j = 0; j < NUM_ROWS; j++) {
//             const uint8_t keycode = keyboard_layout[j][i];
//             const bool is_pressed = keyboard_state[j][i];
//             if (!is_pressed) {
//                 if (is_modifier_key(keycode)) {
//                     _keyboard_state.modifiers &= ~(1 << (keycode & ~0xe0));
//                 } else {
//                     _keyboard_state.pressed_keys[k] = 0;
//                 }
//             } else {
//                 if (is_modifier_key(keycode)) {
//                     _keyboard_state.modifiers |= (1 << (keycode & ~0xe0));
//                 } else {
//                     num_pressed_keys++;
//                     _keyboard_state.pressed_keys[num_pressed_keys] =
//                         keyboard_layout[j][i];
//                 }
//             }
//             k++;
//         }
//     }

//     if (num_pressed_keys > 6) {
//         _keyboard_state.is_overflow = true;
//     }

//     return &_keyboard_state;
// }
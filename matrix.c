#include "matrix.h"

uint8_t keyboard_modifiers = 0;
uint8_t keyboard_keys[] = {0, 0, 0, 0, 0, 0, 0, 0};

#define NUM_ROWS 2
#define NUM_COLS 4
uint8_t keyboard_layout[NUM_ROWS][NUM_COLS] = {{0, 0, 0, 0}, {0, 0, 0, 0}};

void matrix_scan() {
    uint8_t k = 0;
    for (uint8_t i = 0; i < NUM_ROWS; i++) {
        for (uint8_t j = 0; j < NUM_COLS; j++) {
            const uint8_t keycode = keyboard_layout[i][j];
            bool is_pressed = read_pin(i, j);
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
                    keyboard_keys[k] = keyboard_layout[i][j];
                }
            }
            k++;
        }
    }
}
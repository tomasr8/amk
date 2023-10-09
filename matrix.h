#include <stdint.h>
#include <stdbool.h>

__attribute__((always_inline)) static inline bool is_modifier_key(uint8_t keycode) {
    return (keycode >= 0xe0) && (keycode <= 0xe7);
}
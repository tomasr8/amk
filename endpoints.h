#ifndef ENDPOINTS_H
#define ENDPOINTS_H
#include <avr/interrupt.h>
#include <avr/io.h>
#include <stdbool.h>
#include <stdint.h>

#define REQDIR_DEVICETOHOST (1 << 7)

typedef struct {
    uint8_t bmRequestType;
    uint8_t bRequest;
    uint16_t wValue;
    uint16_t wIndex;
    uint16_t wLength;
} SetupRequest_t;

__attribute__((always_inline)) static inline bool is_setup_packet() {
    return (UEINTX & (1 << RXSTPI)) ? true : false;
}

__attribute__((always_inline)) static inline bool is_in_ready() {
    return (UEINTX & (1 << TXINI)) ? true : false;
}

__attribute__((always_inline)) static inline bool is_out_received() {
    return (UEINTX & (1 << RXOUTI)) ? true : false;
}

__attribute__((always_inline)) static inline void clear_setup_flag() {
    UEINTX &= ~(1 << RXSTPI);
}

__attribute__((always_inline)) static inline void clear_in_flag() {
    UEINTX &= ~(1 << TXINI);
}

__attribute__((always_inline)) static inline void clear_out_flag() {
    UEINTX &= ~(1 << RXOUTI);
}

__attribute__((always_inline)) static inline void clear_status_stage(
    uint8_t bmRequestType) {
    if (bmRequestType & REQDIR_DEVICETOHOST) {
        while (!(is_out_received()))
            ;
        clear_out_flag();
    } else {
        while (!(is_in_ready()))
            ;
        clear_in_flag();
    }
}

__attribute__((always_inline, warn_unused_result)) static inline uint8_t
read_byte() {
    return UEDATX;
}

void read_setup_request(SetupRequest_t *request);

bool configure_control_endpoint();
bool configure_keyboard_endpoint();

#endif
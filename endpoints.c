#include "endpoints.h"

void read_setup_request(SetupRequest_t *request) {
    uint8_t *buffer = (uint8_t *)request;
    for (uint8_t i = 0; i < sizeof(SetupRequest_t); i++) {
        buffer[i] = read_byte();
    }
}

void select_control_endpoint() {
    UENUM = 0;
}

void select_keyboard_endpoint() {
    UENUM = 1;
}

bool configure_control_endpoint() {
    UENUM = 0;             // Select Endpoint 0, the default control endpoint
    UECONX = (1 << EPEN);  // Enable the Endpoint
    UECFG0X = 0;  // Control Endpoint, OUT direction for control endpoint
    UECFG1X |= (1 << EPSIZE1) | (1 << EPSIZE0) |
               (1 << ALLOC);  // 64 byte endpoint, 1 bank, allocate the memory

    if (!(UESTA0X &
          (1 << CFGOK))) {  // Check if endpoint configuration was successful
        return false;
    }

    UERST |= (1 << EPRST0);  // Reset Endpoint (potentially unnecessary?)
    UERST &= ~(1 << EPRST0);

    UEIENX = (1 << RXSTPE);  // Enable the Receive Setup Packet Interrupt
    return true;
}

bool configure_keyboard_endpoint() {
    UENUM = 1;             // Select Endpoint 1
    UECONX = (1 << EPEN);  // Enable the Endpoint
    UECFG0X =
        (1 << EPTYPE1) | (1 << EPTYPE0) | (1 << EPDIR);  // Interrup IN endpoint
    UECFG1X |=
        (1 << EPSIZE1) | (1 << EPSIZE0) |
        (1 << ALLOC);  // 64 byte endpoint, single-bank, allocate the memory

    if (!(UESTA0X &
          (1 << CFGOK))) {  // Check if endpoint configuration was successful
        return false;
    }

    UERST |= (1 << EPRST1);  // Reset Endpoint (potentially unnecessary?)
    UERST &= ~(1 << EPRST1);

    return true;
}

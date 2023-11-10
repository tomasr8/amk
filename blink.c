#ifndef F_CPU
#define F_CPU 16000000UL  // 16mhz
#endif

#include "blink.h"

#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/atomic.h>
#include <util/delay.h>

#include "descriptors.h"
#include "endpoints.h"
#include "keys.h"
#include "matrix.h"

volatile bool using_report_protocol = true;
volatile uint8_t current_configuration = 0;
volatile uint16_t keyboard_idle_duration = 500;
volatile uint16_t keyboard_idle_remaining = 0;
volatile bool should_send_report = false;

enum USB_DEVICE_STATE {
    DEFAULT,
    ADDRESSED,
    CONFIGURED,
};

volatile enum USB_DEVICE_STATE usb_device_state = DEFAULT;

static void handle_standard_request(SetupRequest_t *request);
static void handle_hid_request(SetupRequest_t *request);

static void usb_device_get_status(SetupRequest_t *request);
static void usb_device_set_address(SetupRequest_t *request);
static void usb_device_get_descriptor(SetupRequest_t *request);
static void usb_device_get_configuration(SetupRequest_t *request);
static void usb_device_set_configuration(SetupRequest_t *request);
static void hid_get_idle(SetupRequest_t *request);
static void hid_set_idle(SetupRequest_t *request);
static void hid_get_protocol(SetupRequest_t *request);
static void hid_set_protocol(SetupRequest_t *request);

static void send_report();

void usb_init() {
    cli();
    // Enable pads regulator
    UHWCON |= (1 << UVREGE);

    // Configure PLL clock prescaler to 16mhz and enable the clock
    // which is used as the source for the USB clock
    // (Should work even w/o setting the prescaler)
    PLLCSR |= (1 << PINDIV) | (1 << PLLE);
    while (!(PLLCSR & (1 << PLOCK))) {
    }  // Wait for the clock to settle

    // Enable VBUS pad & the USB CONTROLLER itself
    USBCON |= (1 << USBE) | (1 << OTGPADE);

    // Unfreeze USB controller clock
    USBCON &= ~(1 << FRZCLK);

    // Set full speed mode
    UDCON &= ~(1 << LSM);

    // Attach to the bus
    UDCON &= ~(1 << DETACH);

    // Enable USB "End Of Reset Interrupt" and "Start Of Frame Interrupt"
    UDIEN |= (1 << EORSTE) | (1 << SOFE);
    sei();
}

int main(void) {
    usb_init();
    init_pins();
    while (1) {
        bool changed = matrix_scan();

        if ((usb_device_state == CONFIGURED) &&
            (keyboard_idle_remaining == 0)) {
            ATOMIC_BLOCK(ATOMIC_FORCEON) {
                keyboard_state_t state = get_state();

                uint8_t prev_uenum = UENUM;
                UENUM = 1;
                if (endpoint_is_read_write_allowed()) {
                    send_report(state);

                    // uint16_t must be updated atomically because it is also
                    // modified in the ISR
                    keyboard_idle_remaining = keyboard_idle_duration;
                }
                UENUM = prev_uenum;
            }
        }

        // if (usb_device_state == CONFIGURED) {
        // }
    }
}

ISR(USB_GEN_vect) {
    if (UDINT & (1 << EORSTI)) {
        UDINT &= ~(1 << EORSTI);
        bool result = configure_control_endpoint();
    }
    if (UDINT & (1 << SOFI)) {
        UDINT &= ~(1 << SOFI);
        if (keyboard_idle_remaining > 0) {
            keyboard_idle_remaining--;
        }
    }
}

ISR(USB_COM_vect) {
    uint8_t prev_uenum = UENUM;
    UENUM = 0;

    if (is_setup_packet()) {
        SetupRequest_t request;
        read_setup_request(&request);

        handle_hid_request(&request);
        if (!is_setup_packet()) {
            return;
        }
        handle_standard_request(&request);
    }

    // If the RXSTPI flag is still set, it means that the request was not
    // recognized so we stall the endpoint. There is no need to clear the stall
    // (STALLRQC), the hardware does it automatically before the next SETUP
    // packet (see section 22.11.1 of the atmega32u4 datasheet).
    if (is_setup_packet()) {
        clear_setup_flag();
        stall_request();
    }
    UENUM = prev_uenum;
}

static void handle_standard_request(SetupRequest_t *request) {
    const uint8_t bmRequestType = request->bmRequestType;
    const uint8_t bRequest = request->bRequest;

    if (bRequest == GET_STATUS) {
        if ((bmRequestType ==
             (REQDIR_DEVICETOHOST | REQTYPE_STANDARD | REQREC_DEVICE)) ||
            (bmRequestType ==
             (REQDIR_DEVICETOHOST | REQTYPE_STANDARD | REQREC_INTERFACE)) ||
            (bmRequestType ==
             (REQDIR_DEVICETOHOST | REQTYPE_STANDARD | REQREC_ENDPOINT))) {
            usb_device_get_status(request);
        }
    } else if (bRequest == SET_ADDRESS) {
        if (bmRequestType ==
            (REQDIR_HOSTTODEVICE | REQTYPE_STANDARD | REQREC_DEVICE)) {
            usb_device_set_address(request);
        }
    } else if (bRequest == GET_DESCRIPTOR) {
        if ((bmRequestType ==
             (REQDIR_DEVICETOHOST | REQTYPE_STANDARD | REQREC_DEVICE)) ||
            (bmRequestType ==
             (REQDIR_DEVICETOHOST | REQTYPE_STANDARD | REQREC_INTERFACE))) {
            // The last bmRequestType type is not in the USB 2.0 specification,
            // but it is apparently needed (LUFA also uses it,
            // Drivers/USB/Core/DeviceStandardReq.c)
            usb_device_get_descriptor(request);
        }
    } else if (bRequest == GET_CONFIGURATION) {
        if (bmRequestType ==
            (REQDIR_DEVICETOHOST | REQTYPE_STANDARD | REQREC_DEVICE)) {
            usb_device_get_configuration(request);
        }
    } else if (bRequest == SET_CONFIGURATION) {
        if (bmRequestType ==
            (REQDIR_HOSTTODEVICE | REQTYPE_STANDARD | REQREC_DEVICE)) {
            usb_device_set_configuration(request);
        }
    } else if (bRequest == GET_INTERFACE) {
        if (bmRequestType ==
            (REQDIR_DEVICETOHOST | REQTYPE_STANDARD | REQREC_INTERFACE)) {
            // We don't support alternate settings..
            clear_setup_flag();
            write_byte(0);
            clear_in_flag();
            clear_status_stage(request->bmRequestType);
        }
    } else if (bRequest == SET_INTERFACE) {
        if (bmRequestType ==
            (REQDIR_HOSTTODEVICE | REQTYPE_STANDARD | REQREC_INTERFACE)) {
            // USB 2.0 Section 9.4.10: "If a device only supports a default
            // setting for the specified interface, then a STALL may be returned
            // in the Status stage of the request"
        }
    } else if (bRequest == CLEAR_FEATURE) {
        if ((bmRequestType ==
             (REQDIR_HOSTTODEVICE | REQTYPE_STANDARD | REQREC_DEVICE)) ||
            (bmRequestType ==
             (REQDIR_HOSTTODEVICE | REQTYPE_STANDARD | REQREC_INTERFACE)) ||
            (bmRequestType ==
             (REQDIR_HOSTTODEVICE | REQTYPE_STANDARD | REQREC_ENDPOINT))) {
            // Noop as we don't have any features
            clear_setup_flag();
            clear_status_stage(request->bmRequestType);
        }
    } else if (bRequest == SET_FEATURE) {
        if ((bmRequestType ==
             (REQDIR_HOSTTODEVICE | REQTYPE_STANDARD | REQREC_DEVICE)) ||
            (bmRequestType ==
             (REQDIR_HOSTTODEVICE | REQTYPE_STANDARD | REQREC_INTERFACE)) ||
            (bmRequestType ==
             (REQDIR_HOSTTODEVICE | REQTYPE_STANDARD | REQREC_ENDPOINT))) {
            // Noop as we don't have any features
            clear_setup_flag();
            clear_status_stage(request->bmRequestType);
        }
    } else if (bRequest == SYNCH_FRAME) {
        if (bmRequestType ==
            (REQDIR_DEVICETOHOST | REQTYPE_STANDARD | REQREC_ENDPOINT)) {
            // We don't use isochronous endpoints, so we don't need to
            // handle this
            return;
        }
    }
}

static void handle_hid_request(SetupRequest_t *request) {
    const uint8_t bmRequestType = request->bmRequestType;
    const uint8_t bRequest = request->bRequest;

    if (bRequest == GET_PROTOCOL) {
        if (bmRequestType ==
            (REQDIR_DEVICETOHOST | REQTYPE_CLASS | REQREC_INTERFACE)) {
            hid_get_protocol(request);
        }
    } else if (request->bRequest == GET_IDLE) {
        if (bmRequestType ==
            (REQDIR_DEVICETOHOST | REQTYPE_CLASS | REQREC_INTERFACE)) {
            hid_get_idle(request);
        }
    } else if (request->bRequest == GET_REPORT) {
        if (bmRequestType ==
            (REQDIR_DEVICETOHOST | REQTYPE_CLASS | REQREC_INTERFACE)) {
            clear_setup_flag();

            write_byte(0);
            write_byte(0);  // reserved
            if (usb_device_state == CONFIGURED) {
                write_byte(KEY_C);
            } else if (usb_device_state == ADDRESSED) {
                write_byte(KEY_A);
            } else {
                write_byte(KEY_D);
            }
            if (usb_device_state == CONFIGURED) {
                write_byte(KEY_C);
            } else if (usb_device_state == ADDRESSED) {
                write_byte(KEY_A);
            } else {
                write_byte(KEY_D);
            }
            write_byte(0);
            write_byte(0);
            write_byte(0);
            write_byte(0);

            clear_in_flag();
            clear_status_stage(request->bmRequestType);
        }
    } else if (request->bRequest == SET_PROTOCOL) {
        if (bmRequestType ==
            (REQDIR_HOSTTODEVICE | REQTYPE_CLASS | REQREC_INTERFACE)) {
            hid_set_protocol(request);
        }
    } else if (request->bRequest == SET_IDLE) {
        if (bmRequestType ==
            (REQDIR_HOSTTODEVICE | REQTYPE_CLASS | REQREC_INTERFACE)) {
            hid_set_idle(request);
        }
    } else if (request->bRequest == SET_REPORT) {
        if (bmRequestType ==
            (REQDIR_HOSTTODEVICE | REQTYPE_CLASS | REQREC_INTERFACE)) {
            // our HID report has no output fields i.e. nothing can be set
            // on the keyboard
            clear_setup_flag();
            clear_status_stage(request->bmRequestType);
            return;
        }
    }
}

static void usb_device_get_status(SetupRequest_t *request) {
    clear_setup_flag();

    write_byte(0);
    write_byte(0);
    clear_in_flag();

    clear_status_stage(request->bmRequestType);
}

static void usb_device_set_address(SetupRequest_t *request) {
    uint8_t address =
        request->wValue &
        0x7F;  // usb uses 7-bit addresses (i.e. max address is 127)

    UDADDR = address;

    clear_setup_flag();
    clear_status_stage(request->bmRequestType);

    while (!(is_in_ready()))
        ;

    // The new address cannot be enabled before the status stage has
    // completed, otherwise the status stage would fail. We must first clear
    // the status stage and only then enable the address.
    UDADDR |= (1 << ADDEN);
    usb_device_state = ADDRESSED;
}

static void usb_device_get_descriptor(SetupRequest_t *request) {
    // The Host is requesting a descriptor to enumerate the device
    const uint8_t descriptor_type = request->wValue >> 8;
    uint8_t *descriptor;
    uint8_t descriptor_length;

    if (descriptor_type == DESCRIPTOR_DEVICE) {
        descriptor = (uint8_t *)&device_descriptor;
        descriptor_length = sizeof(device_descriptor);
    } else if (descriptor_type == DESCRIPTOR_CONFIGURATION) {
        // The reponse to configuration request sends the entire
        // confdiguration tree including interface, vendor (HID) and
        // endpoint descriptors (not including the the HID report
        // descriptor)
        descriptor = (uint8_t *)&configuration_descriptor;
        descriptor_length = sizeof(configuration_descriptor);
    } else if (descriptor_type == DESCRIPTOR_CLASS_HID) {  // HID descriptor
        descriptor = (uint8_t *)&configuration_descriptor.hid;
        descriptor_length = sizeof(configuration_descriptor.hid);
    } else if (descriptor_type ==
               DESCRIPTOR_CLASS_REPORT) {  // HID report descriptor
        descriptor = (uint8_t *)hid_report_descriptor;
        descriptor_length = 45;
    } else {
        // something else we don't know how to respond to
        return;
    }

    uint8_t length = request->wLength;
    if (request->wLength < descriptor_length) {
        length = request->wLength;
    }

    clear_setup_flag();

    while (length > 0) {
        if (is_out_received()) {
            break;
        }  // If there is another packet, exit to handle it

        if (!(is_in_ready())) {
            continue;
        }

        int i = 0;
        while ((length > 0) && (i < 64)) {
            UEDATX = pgm_read_byte(descriptor++);
            length--;
            i++;
        }

        clear_in_flag();
    }

    clear_status_stage(request->bmRequestType);
}

static void usb_device_get_configuration(SetupRequest_t *request) {
    uint8_t value = (uint8_t)request->wValue;
    if (value > 1) {
        return;
    }

    clear_setup_flag();

    write_byte(current_configuration);
    clear_in_flag();

    clear_status_stage(request->bmRequestType);
}

static void usb_device_set_configuration(SetupRequest_t *request) {
    uint8_t value = (uint8_t)request->wValue;
    if (value > 1) {
        return;
    }

    clear_setup_flag();

    current_configuration = value;

    clear_status_stage(request->bmRequestType);

    if (usb_device_state == ADDRESSED) {
        bool result = configure_keyboard_endpoint();
        if (result) {
            usb_device_state = CONFIGURED;
        }
    }
    // reset idle duration back to default
    keyboard_idle_duration = 500;
    keyboard_idle_remaining = 0;
}

static void hid_get_idle(SetupRequest_t *request) {
    clear_setup_flag();

    UEDATX =
        keyboard_idle_remaining >> 2;  // The value is in increments of 4ms,
                                       // so we need to divide by 4 first

    clear_in_flag();
    clear_status_stage(request->bmRequestType);
}

static void hid_set_idle(SetupRequest_t *request) {
    // The value is in increments of 4ms, so we multiply by 4 to get the
    // milliseconds
    const uint8_t idle = (uint8_t)((request->wValue & 0xFF00) >> 6);

    clear_setup_flag();

    keyboard_idle_duration = idle;

    clear_status_stage(request->bmRequestType);
}

static void hid_get_protocol(SetupRequest_t *request) {
    clear_setup_flag();
    write_byte((uint8_t)using_report_protocol);
    clear_in_flag();
    clear_status_stage(request->bmRequestType);
}

static void hid_set_protocol(SetupRequest_t *request) {
    clear_setup_flag();
    using_report_protocol = request->wValue == 1 ? true : false;
    clear_status_stage(request->bmRequestType);
}

// static void hid_send_report(SetupRequest_t *request) {
//     uint8_t length = request->wLength;
//     if (request->wLength < 18) {
//         length = request->wLength;
//     }

//     clear_setup_flag();
//     uint8_t num_pressed_keys = get_pressed_keys();

//     write_byte(keyboard_modifiers);
//     write_byte(0);  // reserved
//     for (uint8_t i = 0; i < 6; i++) {
//         if (num_pressed_keys > 6) {
//             write_byte(KEY_ERR_OVF);
//         } else {
//             write_byte(keyboard_pressed_keys[i]);
//         }
//     }

//     write_byte(keyboard_modifiers);
//     write_byte(0);  // reserved
//     for (uint8_t i = 0; i < 8; i++) {
//         write_byte(keyboard_pressed_keys[i]);
//     }

//     clear_in_flag();
//     clear_status_stage(request->bmRequestType);
// }

static void send_report(keyboard_state_t state) {
    uint8_t idle = keyboard_idle_duration >> 2;

    write_byte(state.modifiers);
    write_byte(idle);  // reserved
    write_byte(state.pressed_keys[0]);
    write_byte(state.pressed_keys[1]);
    write_byte(state.pressed_keys[2]);
    write_byte(state.pressed_keys[3]);
    write_byte(state.pressed_keys[4]);
    write_byte(state.pressed_keys[5]);
    // clear_in_flag();
    UEINTX = 0b00111010;
}
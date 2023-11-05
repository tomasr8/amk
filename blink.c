#ifndef F_CPU
#define F_CPU 16000000UL  // 16mhz
#endif

#include "blink.h"

#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>

#include "descriptors.h"
#include "endpoints.h"

bool using_report_protocol = true;
uint8_t current_configuration = 0;
uint16_t keyboard_idle_duration = 500;

enum USB_DEVICE_STATE {
    DEFAULT,
    ADDRESSED,
    CONFIGURED,
};

enum USB_DEVICE_STATE usb_device_state = DEFAULT;

static void usb_device_set_address(SetupRequest_t *request);
static void usb_device_get_descriptor(SetupRequest_t *request);
static void usb_device_get_configuration(SetupRequest_t *request);
static void usb_device_set_configuration(SetupRequest_t *request);
static void hid_get_idle(SetupRequest_t *request);
static void hid_set_idle(SetupRequest_t *request);
static void hid_get_protocol(SetupRequest_t *request);
static void hid_set_protocol(SetupRequest_t *request);

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

    // Enable VBUS pad & USB CONTROLLER itself
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
    while (1) {
    }
}

ISR(USB_GEN_vect) {
    if (UDINT & (0x01 << EORSTI)) {
        UDINT &= ~(0x01 << EORSTI);
        bool result = configure_control_endpoint();
    }
}

ISR(USB_COM_vect) {
    UENUM = 0;

    if (is_setup_packet()) {
        SetupRequest_t request;
        read_setup_request(&request);

        if (request.bRequest == SET_ADDRESS) {
            usb_device_set_address(&request);
            return;
        } else if (request.bRequest == GET_DESCRIPTOR) {
            usb_device_get_descriptor(&request);
            return;
        } else if (request.bRequest == GET_CONFIGURATION) {
            usb_device_get_configuration(&request);
            return;
        } else if (request.bRequest == SET_CONFIGURATION) {
            usb_device_set_configuration(&request);
            return;
        } else if (request.bRequest == GET_INTERFACE) {
            return;
        } else if (request.bRequest == SET_INTERFACE) {
            return;
        } else if (request.bRequest == SYNCH_FRAME) {
            // only for isochronous endpoints which we don't use
            return;
        } else if (request.bRequest == GET_PROTOCOL) {
            hid_get_protocol(&request);
            return;
        } else if (request.bRequest == SET_PROTOCOL) {
            hid_set_protocol(&request);
            return;
        } else if (request.bRequest == GET_REPORT) {
            // TODO
            return;
        } else if (request.bRequest == SET_REPORT) {
            // our HID report has no output fields i.e. nothing can be set on
            // the keyboard
            return;
        } else if (request.bRequest == GET_IDLE) {
            hid_get_idle(&request);
            return;
        } else if (request.bRequest == SET_IDLE) {
            hid_set_idle(&request);
            return;
        }
    }

    // If we the RXSTPI flag is still set, it means that the request was not
    // recognized We stall the endpoint. There is no need to clear the stall
    // (STALLRQC), the hardware does it automatically before the next SETUP
    // packet (see section 22.11.1 of the atmega32u4 datasheet).
    if (is_setup_packet()) {
        clear_setup_flag();
        stall_request();
    }
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

    // The new address accanot be enabled before the status stage has completed,
    // otherwise the status stage would fail. We must first set the status stage
    // and only then enable the address.
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
        // The reponse to configuration request sends the entire confdiguration
        // tree including interface, vendor (HID) and endpoint descriptors (not
        // including the the HID report descriptor)
        descriptor = (uint8_t *)&configuration_descriptor;
        descriptor_length = sizeof(configuration_descriptor);
    } else if (descriptor_type == DESCRIPTOR_CLASS_HID) {  // HID descriptor
        descriptor = (uint8_t *)&configuration_descriptor.hid;
        descriptor_length = sizeof(configuration_descriptor.hid);
    } else if (descriptor_type ==
               DESCRIPTOR_CLASS_REPORT) {  // HID report descriptor
        descriptor = (uint8_t *)&hid_report_descriptor;
        descriptor_length = sizeof(hid_report_descriptor);
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

    if (value == 0 && usb_device_state == CONFIGURED) {
        usb_device_state = ADDRESSED;
    } else if (value != 0 && usb_device_state == ADDRESSED) {
        configure_keyboard_endpoint();
        usb_device_state = CONFIGURED;
    }
}

static void hid_get_idle(SetupRequest_t *request) {
    clear_setup_flag();

    UEDATX = keyboard_idle_duration >> 2;  // The value is in increments of 4ms,
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
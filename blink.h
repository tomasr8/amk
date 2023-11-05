/*
usb.h
*/
#ifndef USB_H
#define USB_H
#include <stdbool.h>
#include <stdint.h>

extern volatile uint8_t keyboard_pressed_keys[6];
extern volatile uint8_t keyboard_modifier;

void usb_init();
int usb_send();
int send_keypress(uint8_t, uint8_t);

#define GET_STATUS 0x00
#define CLEAR_FEATURE 0x01
#define SET_FEATURE 0x03
#define SET_ADDRESS 0x05
#define GET_DESCRIPTOR 0x06
#define GET_CONFIGURATION 0x08
#define SET_CONFIGURATION 0x09
#define GET_INTERFACE 0x0A
#define SET_INTERFACE 0x0B
#define SYNCH_FRAME 0x0C

#define HID_OFFSET 18

// HID Class-specific request codes - refer to HID Class Specification
// Chapter 7.2 - Remarks

#define GET_REPORT 0x01
#define GET_IDLE 0x02
#define GET_PROTOCOL 0x03
#define SET_REPORT 0x09
#define SET_IDLE 0x0A
#define SET_PROTOCOL 0x0B

// USB 2.0 Specification table 9-5
#define DESCRIPTOR_DEVICE 1
#define DESCRIPTOR_CONFIGURATION 2
#define DESCRIPTOR_STRING 3
#define DESCRIPTOR_INTERFACE 4
#define DESCRIPTOR_ENDPOINT 5
#define DESCRIPTOR_DEVICE_QUALIFIER 6
#define DESCRIPTOR_OTHER_SPEED_CONFIGURATION 7
#define DESCRIPTOR_INTERFACE_POWER 8

// HID 1.1 Chapter 7.1
#define DESCRIPTOR_CLASS_HID 0x21
#define DESCRIPTOR_CLASS_REPORT 0x22

#endif
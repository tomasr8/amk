#include "descriptors.h"

USB_DeviceDescriptor_t device_descriptor = {
    .bLength = 0x12,
    .bDescriptorType = 0x01,
    .bcdUSB = 0x100,
    .bDeviceClass = 0x00,
    .bDeviceSubClass = 0x00,
    .bDeviceProtocol = 0x00,
    .bMaxPacketSize0 = 0x08,
    .idVendor = 0xFFFF,
    .idProduct = 0x0001,
    .bcdDevice = 0x0100,
    .iManufacturer = 0x00,
    .iProduct = 0x00,
    .iSerialNumber = 0x00,
    .bNumConfigurations = 0x01
};

USB_ConfigurationDescriptor_t configuration_descriptor = {
    .bLength = 0x09,
    .bDescriptorType = 0x02,
    .wTotalLength = 0x00,
    .bNumInterfaces = 0x02,
    .bConfigurationValue = 0x01,
    .iConfiguration = 0x00,
    .bmAttributes = 0b10100000,
    .bMaxPower = 0x20
};

USB_InterfaceDescriptor_t interface_descriptor = {
    .bLength = 0x09,
    .bDescriptorType = 0x04,
    .bInterfaceNumber = 0x00,
    .bAlternateSetting = 0x00,
    .bNumEndpoints = 0x01,
    .bInterfaceClass = 0x03,
    .bInterfaceSubClass = 0x01,
    .bInterfaceProtocol = 0x01,
    .iInterface = 0x00
};

USB_HIDDescriptor_t hid_descriptor = {
    .bLength = 0x09,
    .bDescriptorType = 0x21,
    .bcdHID = 0x101,
    .bCountryCode = 0x00,
    .bNumDescriptors = 0x01,
    .bDescriptorType = 0x22,
    .wDescriptorLength = 0x3F
};

USB_EndpointDescriptor_t endpoint_descriptor = {
    .bLength = 0x07,
    .bDescriptorType = 0x05,
    .bEndpointAddress = 0b10000001,
    .bmAttributes = 0b00000011,
    .wMaxPacketSize = 0x0008,
    .bInterval = 0x0A
};

static const uint8_t hid_report_descriptor[] PROGMEM = {
    0x05,
    0x01,  // Usage Page - Generic Desktop - HID Spec Appendix E E.6 - The
           // values for the HID tags are not clearly listed anywhere really, so
           // this table is very useful
    0x09,
    0x06,  // Usage - Keyboard
    0xA1,
    0x01,  // Collection - Application
    0x05,
    0x07,  // Usage Page - Key Codes
    0x19,
    0xE0,  // Usage Minimum - The bit that controls the 8 modifier characters
           // (ctrl, command, etc)
    0x29,
    0xE7,  // Usage Maximum - The end of the modifier bit (0xE7 - 0xE0 = 1 byte)
    0x15,
    0x00,  // Logical Minimum - These keys are either not pressed or pressed, 0
           // or 1
    0x25,
    0x01,  // Logical Maximum - Pressed state == 1
    0x75,
    0x01,  // Report Size - The size of the IN report to the host
    0x95,
    0x08,  // Report Count - The number of keys in the report
    0x81,
    0x02,  // Input - These are variable inputs
    0x95,
    0x01,  // Report Count - 1
    0x75,
    0x08,  // Report Size - 8
    0x81,
    0x01,  // This byte is reserved according to the spec
    0x95,
    0x05,  // Report Count - This is for the keyboard LEDs
    0x75,
    0x01,  // Report Size
    0x05,
    0x08,  // Usage Page for LEDs
    0x19,
    0x01,  // Usage minimum for LEDs
    0x29,
    0x05,  // Usage maximum for LEDs
    0x91,
    0x02,  // Output - This is for a host output to the keyboard for the status
           // of the LEDs
    0x95,
    0x01,  // Padding for the report so that it is at least 1 byte
    0x75,
    0x03,  // Padding
    0x91,
    0x01,  // Output - Constant for padding
    0x95,
    0x06,  // Report Count - For the keys
    0x75,
    0x08,  // Report Size - For the keys
    0x15,
    0x00,  // Logical Minimum
    0x25,
    0x65,  // Logical Maximum
    0x05,
    0x07,  // Usage Page - Key Codes
    0x19,
    0x00,  // Usage Minimum - 0
    0x29,
    0x65,  // Usage Maximum - 101
    0x81,
    0x00,  // Input - Data, Array
    0xC0   // End collection
};
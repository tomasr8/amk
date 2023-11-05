#include <avr/io.h>
#include <avr/pgmspace.h>
#include <stdint.h>

// http://www.linux-usb.org/usb.ids
#define IDVENDOR 0x03eb   // Atmel Corp.
#define IDPRODUCT 0x2ff4  // ATMega32u4 DFU Bootloader

typedef struct {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint16_t bcdUSB;
    uint8_t bDeviceClass;
    uint8_t bDeviceSubClass;
    uint8_t bDeviceProtocol;
    uint8_t bMaxPacketSize0;
    uint16_t idVendor;
    uint16_t idProduct;
    uint16_t bcdDevice;
    uint8_t iManufacturer;
    uint8_t iProduct;
    uint8_t iSerialNumber;
    uint8_t bNumConfigurations;
} __attribute__((packed)) USB_DeviceDescriptor_t;

typedef struct {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint16_t wTotalLength;
    uint8_t bNumInterfaces;
    uint8_t bConfigurationValue;
    uint8_t iConfiguration;
    uint8_t bmAttributes;
    uint8_t bMaxPower;
} __attribute__((packed)) USB_ConfigurationDescriptor_t;

typedef struct {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint8_t bInterfaceNumber;
    uint8_t bAlternateSetting;
    uint8_t bNumEndpoints;
    uint8_t bInterfaceClass;
    uint8_t bInterfaceSubClass;
    uint8_t bInterfaceProtocol;
    uint8_t iInterface;
} __attribute__((packed)) USB_InterfaceDescriptor_t;

typedef struct {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint16_t bcdHID;
    uint8_t bCountryCode;
    uint8_t bNumDescriptors;
    uint8_t bReportDescriptorType;
    uint16_t wDescriptorLength;
} __attribute__((packed)) USB_HIDDescriptor_t;

typedef struct {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint8_t bEndpointAddress;
    uint8_t bmAttributes;
    uint16_t wMaxPacketSize;
    uint8_t bInterval;
} __attribute__((packed)) USB_EndpointDescriptor_t;

typedef struct {
    USB_ConfigurationDescriptor_t configration;
    USB_InterfaceDescriptor_t interface;
    USB_HIDDescriptor_t hid;
    USB_EndpointDescriptor_t endpoint;
} USB_Configuration_t;

typedef uint8_t USB_HIDReportDescriptor_t;

const USB_DeviceDescriptor_t device_descriptor PROGMEM = {
    .bLength = 0x12,
    .bDescriptorType = 0x01,
    .bcdUSB = 0x200,
    .bDeviceClass = 0x00,
    .bDeviceSubClass = 0x00,
    .bDeviceProtocol = 0x00,
    .bMaxPacketSize0 = 0x40,  // 64
    .idVendor = IDVENDOR,
    .idProduct = IDPRODUCT,
    .bcdDevice = 0x0100,
    .iManufacturer = 0x00,
    .iProduct = 0x00,
    .iSerialNumber = 0x00,
    .bNumConfigurations = 0x01};

const USB_Configuration_t configuration_descriptor PROGMEM = {
    .configration = {.bLength = 0x09,
                     .bDescriptorType = 0x02,
                     .wTotalLength = 0x22,
                     .bNumInterfaces = 0x01,
                     .bConfigurationValue = 0x01,
                     .iConfiguration = 0x00,
                     .bmAttributes = 0b10100000,
                     .bMaxPower = 0x20},
    .interface = {.bLength = 0x09,
                  .bDescriptorType = 0x04,
                  .bInterfaceNumber = 0x00,
                  .bAlternateSetting = 0x00,
                  .bNumEndpoints = 0x01,
                  .bInterfaceClass = 0x03,
                  .bInterfaceSubClass = 0x01,
                  .bInterfaceProtocol = 0x01,
                  .iInterface = 0x00},
    .hid =
        {
            .bLength = 0x09,
            .bDescriptorType = 0x21,
            .bcdHID = 0x101,
            .bCountryCode = 0x00,
            .bNumDescriptors = 0x01,
            .bReportDescriptorType = 0x22,
            .wDescriptorLength = 0x33  // size of the HID report descriptor
        },
    .endpoint = {.bLength = 0x07,
                 .bDescriptorType = 0x05,
                 .bEndpointAddress = 0b10000001,
                 .bmAttributes = 0b00000011,
                 .wMaxPacketSize = 0x40,  // 64
                 .bInterval = 0x0A}};

// Boot protocol compatible report descriptor
// See https://www.devever.net/~hl/usbnkro
static const USB_HIDReportDescriptor_t hid_report_descriptor[] PROGMEM = {
    0x05, 0x01,  // Usage Page - Generic Desktop - HID Spec Appendix E E.6 - The
                 // values for the HID tags are not clearly listed anywhere
                 // really, so this table is very useful
    0x09, 0x06,  // Usage - Keyboard
    0xA1, 0x01,  // Collection - Application
    0x95, 0x08,  // Report Count - 1
    0x75, 0x08,  // Report Size - 8
    0x91, 0x01,  // Padding

    // <--------------------------------------------->

    0x05, 0x07,  // Usage Page - Key Codes
    0x19, 0xE0,  // Usage Minimum - The bit that controls the 8 modifier
                 // characters (ctrl, command, etc)
    0x29,
    0xE7,  // Usage Maximum - The end of the modifier bit (0xE7 - 0xE0 = 1 byte)
    0x15, 0x00,  // Logical Minimum - These keys are either not pressed or
                 // pressed, 0 or 1
    0x25, 0x01,  // Logical Maximum - Pressed state == 1
    0x75, 0x01,  // Report Size - The size of the IN report to the host
    0x95, 0x08,  // Report Count - The number of keys in the report
    0x81, 0x02,  // Input (Data, Variable, Absolute) ;Modifier byte

    0x95, 0x01,  // Report Count - 1
    0x75, 0x08,  // Report Size - 8
    0x81, 0x01,  // This byte is reserved according to the spec

    0x95, 0x06,  // Report Count - For the keys
    0x75, 0x08,  // Report Size - For the keys
    0x15, 0x00,  // Logical Minimum
    0x25, 0x65,  // Logical Maximum
    0x05, 0x07,  // Usage Page - Key Codes
    0x19, 0x00,  // Usage Minimum - 0
    0x29, 0x65,  // Usage Maximum - 101
    0x81, 0x00,  // Input - Data, Array ;Key array (6 bytes)
    0xC0         // End collection
};

void send_report() {}
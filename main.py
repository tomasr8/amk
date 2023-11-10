import usb.core
import usb.util
import time


def hid_get_report(dev):
    """ Implements HID GetReport via USB control transfer """
    return dev.ctrl_transfer(
        0xA1,  # REQUEST_TYPE_CLASS | RECIPIENT_INTERFACE | ENDPOINT_IN
        2,     # GET_REPORT
        0x200, # "Vendor" Descriptor Type + 0 Descriptor Index
        0,     # USB interface № 0
        64     # max reply size
    )


dev = usb.core.find(idVendor=0x03eb, idProduct=0x2ff4)
ep = dev[0].interfaces()[0].endpoints()[0]

i = dev[0].interfaces()[0].bInterfaceNumber

dev.reset()

if dev.is_kernel_driver_active(i):
    dev.detach_kernel_driver(i)

dev.set_configuration()
print(dev.get_active_configuration())

for i in range(1000):
    print(i, hid_get_report(dev))
    try:
        print(dev.read(0x81, 8, 100))
    except usb.core.USBTimeoutError:
        pass
    time.sleep(0.25)
# while True:
#     try:
#         print(dev.read(0x81, 8, 100))
#     except usb.core.USBTimeoutError:
#         pass

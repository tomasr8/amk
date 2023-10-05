#ifndef F_CPU
#define F_CPU 16000000UL  // or whatever may be your frequency
#endif

#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/delay.h>

void usb_init();

int main(void) {
  usb_init();

  // DDRB = 0xFF;  // set all PORTB pins to output
  // while (1) {
  //   // LED on
  //   PORTB = 0xFF;    // Turn on all PBx pins
  //   _delay_ms(500);  // wait 500 milliseconds

  //   // LED off
  //   PORTB = 0x00;    // Turn off all PBx pins
  //   _delay_ms(500);  // wait 500 milliseconds
  // }
}

void usb_init() {
  // cli();
  // Enable pads regulator
  UHWCON |= (0x01 << UVREGE);

  // Configure clock
  PLLCSR |= (0x01 << PLLE); // Set PLL Enable bit
  PLLFRQ |= (0x01 << PDIV2); // Set PLL frequency to 48mhz
  while (!(PLLCSR & (0x01 << PLOCK))) // Wait for the clock to settle
    ;

  // Enable VBUS pad & USB CONTROLLER
  USBCON |= (1 << USBE) | (1 << OTGPADE);

  // Unfreeze clock
  USBCON &= ~(0x01 << FRZCLK);

  // Full speed mode
  UDCON &= ~(1 << LSM);

  // Attach to the bus
  UDCON &= ~(0x01 << DETACH);
  // sei();
}

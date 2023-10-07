#!/bin/bash

avr-gcc -g -Os -mmcu=atmega32u4 -c -Wall usb.c
avr-gcc -g -mmcu=atmega32u4 -o usb.elf usb.o
avr-objcopy -j .text -j .data -O ihex usb.elf usb.hex
avr-size --format=avr --mcu=atmega32u4 usb.elf 
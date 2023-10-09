compile: clean
	avr-gcc -g -Os -mmcu=atmega32u4 -c -Wall blink.c endpoints.c
	avr-gcc -g -mmcu=atmega32u4 -o blink.elf blink.o endpoints.o
	avr-objcopy -j .text -j .data -O ihex blink.elf blink.hex
	avr-size --format=avr --mcu=atmega32u4 blink.elf

flash: compile
	avrdude -v -c avr109 -p atmega32u4 -P /dev/ttyACM0 -b 57600 -D -U flash:w:blink.hex

clean:
	rm -f *.o *.elf rm *.hex


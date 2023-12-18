DEVICE = atmega328p
AVRDUDE = avrdude -c arduino_as_isp -P /dev/ttyUSB0 -p $(DEVICE) -b 19200
COMPILE = avr-gcc -Wall -Os -Iusbdrv -I. -mmcu=$(DEVICE) -DF_CPU=16000000
COMPILECXX = avr-g++ -Wall -Os -Iusbdrv -I. -mmcu=$(DEVICE) -DF_CPU=16000000

OBJECTS = usbdrv/usbdrvasm.o usbdrv/usbdrv.o usbdrv/oddebug.o screen.o main.o

all:	main.hex

.c.o:
	$(COMPILE) -c $< -o $@

.S.o:
	$(COMPILE) -x assembler-with-cpp -c $< -o $@

.cpp.o:
	$(COMPILECXX) -c $< -o $@

.c.s:
	$(COMPILE) -S $< -o $@

flash:	all
	$(AVRDUDE) -U flash:w:main.hex:i

clean:
	rm -f main.hex main.lst main.obj main.cof main.list main.map main.eep.hex main.bin *.o usbdrv/*.o main.s usbdrv/oddebug.s usbdrv/usbdrv.s

main.bin:	$(OBJECTS)
	$(COMPILE) -o main.bin $(OBJECTS)

main.hex:	main.bin
	rm -f main.hex main.eep.hex
	avr-objcopy -j .text -j .data -O ihex main.bin main.hex

disasm:	main.bin
	avr-objdump -d main.bin
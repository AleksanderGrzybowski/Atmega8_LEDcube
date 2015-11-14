F_CPU := 16000000L

all: flash

clean:
	rm -rf target
	mkdir target

compile: 
	avr-gcc -mmcu=atmega8a -DF_CPU=$(F_CPU) -Wall -Os -o target/out.elf *.c
	avr-objcopy -j .text -j .data -O ihex target/out.elf target/out.hex

flash: clean compile
	# try adding -B 4 if too fast
	sudo avrdude  -p m8 -c usbasp -P usb -U flash:w:target/out.hex






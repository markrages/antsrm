NAME=srm
CC=msp430-gcc
#CFLAGS=-mmcu=msp430g2231 -g -Wall -Os
CFLAGS=-mmcu=msp430g2452 -g -Wall -Os
#CFLAGS=-mmcu=msp430g2302 -g -Wall -Os

OBJECTS=main.o ant.o
all: $(NAME).hex $(NAME).lst

$(NAME).hex: $(NAME).elf
	msp430-objcopy -O ihex $(NAME).elf $(NAME).hex

$(NAME).lst: $(NAME).elf
	msp430-objdump -DS $(NAME).elf > $(NAME).lst
	fromdos $(NAME).lst

$(NAME).elf: main.c generated_messages.h
	$(CC) $(CFLAGS) -o $(NAME).elf main.c

TAGS: *.c *.h
	etags *.c *.h

generated_messages.h: ./qapy/quarqd_messages.py
	echo | ./qapy/quarqd_messages.py --header > generated_messages.h

unicompile:
	$(CC) $(CFLAGS) -fwhole-program -combine -o $(NAME).elf $(C_SOURCES)
	#$(CC) $(CFLAGS) -o $(NAME).elf $(C_SOURCES)

burn: $(NAME).elf 
	# killall msp430-gdbproxy ; sleep 1
	cmp $(NAME).elf .last.elf || sudo mspdebug rf2500 "prog $(NAME).elf"
	cp $(NAME).elf .last.elf

clean:
	rm -f $(OBJECTS)
	rm -f $(NAME).elf $(NAME).hex $(NAME).lst
	rm -f generated_messages.h
	rm -f .last.elf
	rm -f *~



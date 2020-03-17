CC=gcc
CFLAGS=-Wall -ggdb -O0 -g3
INCLUDE=-I/usr/include/modbus
LDFLAGS=-lmodbus -lpthread
OBJECTS=zone1.o uart.o doors.o

ifeq (${DEBUG}, 1)
	CLFAGS+=-ggdb -O0 -g3
endif

zone1.elf: ${OBJECTS}
	${CC} ${CFLAGS} ${OBJECTS} ${LDFLAGS} -o zone1.elf

zone1.o: zone1.c zone1.h uart.h
	${CC} ${CFLAGS} ${INCLUDE} -c zone1.c

uart.o: uart.h
	${CC} ${CFLAGS} -c uart.c

doors.o: doors.c doors.h
	${CC} ${CFLAGS} ${INCLUDE} -c doors.c

clean:
	rm -rf *.o *.elf

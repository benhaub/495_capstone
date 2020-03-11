CC=gcc
CFLAGS=-Wall -ggdb -O0 -g3
INCLUDE=-I/usr/include/modbus
LDFLAGS=-lmodbus
OBJECTS=modbus_rcvr.o uart.o

ifdef DEBUG
	CLFAGS+=-ggdb -O0 -g3
endif

mbrcv.elf: ${OBJECTS}
	${CC} ${CFLAGS} ${OBJECTS} ${LDFLAGS} -o mbrcv.elf

modbus_rcvr.o: modbus_rcvr.c modbus_rcvr.h uart.h
	${CC} ${CFLAGS} ${INCLUDE} -c modbus_rcvr.c

uart.o: uart.h
	${CC} ${CFLAGS} -c uart.c

clean:
	rm -rf *.o *.elf

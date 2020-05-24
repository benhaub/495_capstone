##########################################################################
# Authour : Ben Haubrich                                                 #
# File    : makefile                                                     #
# Synopsis: makefile for EE/CME MST Capstone Design Project              #
##########################################################################
CC=gcc
CFLAGS=-Wall -ggdb -O0 -g3
INCLUDE=-I/usr/include/modbus
LDFLAGS=-lmodbus -lpthread
OBJECTS=fans.o uart.o doors.o

ifeq (${DEBUG}, 1)
	CLFAGS+=-ggdb -O0 -g3
endif

mst_scada.elf: ${OBJECTS}
	${CC} ${CFLAGS} ${OBJECTS} ${LDFLAGS} -o mst_scada.elf

fans.o: fans.c fans.h uart.h
	${CC} ${CFLAGS} ${INCLUDE} -c fans.c

uart.o: uart.h
	${CC} ${CFLAGS} -c uart.c

doors.o: doors.c doors.h
	${CC} ${CFLAGS} ${INCLUDE} -c doors.c

clean:
	rm -rf *.o *.elf

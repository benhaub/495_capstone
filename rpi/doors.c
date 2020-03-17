/* Standard Headers */
#include <string.h> /* for memcpy() */
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
/* Local Headers */
#include "doors.h"
/* Non-standard headers */
#include <modbus.h>

/* From zone1.c */
extern int uartfd;
extern modbus_t *mb_door;

void *door_control(void) {
/* open1, open2, etc are flags that tell us the door has been opene. */
  int open1, open2, open3, open4;
  open1 = open2 = open3 = open4 = 0;
  char *ruletosend;
  uint16_t rc, i, j;
  uint16_t *dest = malloc(sizeof(uint16_t));
  printf("door_control dest addr %p, i addr %p\n", dest, &i);
  j = DOOR1;
  while(1) {
    if(j > NUM_DOORS+DOOR1-1) {
      j = DOOR1;
    }
/* Tell the slave (ICA) to read the jth holding register to dest. */
    if(-1 == (rc = modbus_read_registers(mb_door,j,1, dest))) {
      fprintf(stderr, "zone1 modbus_read_failed: %s\n", modbus_strerror(errno));
      return NULL;
    }
    if(DOOR1 == j && 0 == open1) {
      for(i = 0; i < rc; i++) {
        if(dest[i] != 0) {
          printf("DOOR1: reg[%d]=%u (0x%X)\n", i, *dest, *dest);
          open1 = 1;
          ruletosend = D1OPEN;
          write(uartfd, ruletosend, sizeof(char));
          write(uartfd, ruletosend+1, sizeof(char));
        }
      }
    }
    if(DOOR2 && open2 == 0) {
      open2 = 1;
    }
    if(DOOR3 && open3 == 0) {
      open3 = 1;
    }
    if(DOOR4 && open4 == 0) {
      open4 = 1;
    }
    if(~DOOR1 && open1 == 1) {
      for(i = 0; i < rc; i++) {
        if(dest[i] != 0) {
          open1 = 0;
          ruletosend = D1CLOSE;
          write(uartfd, ruletosend, sizeof(char));
          write(uartfd, ruletosend+1, sizeof(char));
        }
      }
    }
    if(~DOOR2 && open2 == 1) {
    }
    if(~DOOR3 && open3 == 1) {
    }
    if(~DOOR4 && open4 == 1) {
    }
    j++;
  }/*while(1)*/
}


/**************************************************************************
 * Authour : Ben Haubrich                                                 *
 * File    : doors.c                                                      *
 * Synopsis: door control for EE/CME MST Capstone Design Project          *
 * ************************************************************************/
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

/* From fans.c */
extern int uartfd;
extern modbus_t *mb_door;

void *door_control(void) {
/* open1, open2, etc are flags that tell us the door has been opene. */
  int open1, open2, open3, open4;
  open1 = open2 = open3 = open4 = 0;
  char *ruletosend;
  uint16_t rc, i, j;
  uint16_t *dest = malloc(sizeof(uint16_t));
  j = DOOR1;
  while(1) {
    if(j > NUM_DOORS+DOOR1-1) {
      j = DOOR1;
    }
/* Tell the slave (ICA) to read the jth holding register to dest. */
    if(-1 == (rc = modbus_read_registers(mb_door,j,1, dest))) {
      fprintf(stderr, "fans modbus_read_failed: %s\n", modbus_strerror(errno));
      return NULL;
    }
    if(DOOR1 == j && 0 == open1) {
      for(i = 0; i < rc; i++) {
        if(dest[i] != 0) {
          printf("DOOR1: reg[%d]=%u (0x%X)\n", i, *dest, *dest);
          open1 = 1;
          ruletosend = D1OPEN;
          printf("opening. ruletosend: %s\n", ruletosend);
          write(uartfd, ruletosend, sizeof(char));
          usleep(250);
          write(uartfd, ruletosend+1, sizeof(char));
        }
      }
    }
    if(DOOR2 == j && open2 == 0) {
      open2 = 1;
    }
    if(DOOR3 == j && open3 == 0) {
      open3 = 1;
    }
    if(DOOR4 == j && open4 == 0) {
      open4 = 1;
    }
    if(DOOR1 == j && open1 == 1) {
      for(i = 0; i < rc; i++) {
        if(dest[i] == 0) {
          printf("DOOR1: reg[%d]=%u (0x%X)\n", i, *dest, *dest);
          open1 = 0;
          ruletosend = D1CLOSE;
          printf("closing. ruletosend: %s\n", ruletosend);
          write(uartfd, ruletosend, sizeof(char));
          write(uartfd, ruletosend+1, sizeof(char));
        }
      }
    }
    if(DOOR2 == j && open2 == 1) {
    }
    if(DOOR3 == j && open3 == 1) {
    }
    if(DOOR4 == j && open4 == 1) {
    }
    j++;
  }/*while(1)*/
}


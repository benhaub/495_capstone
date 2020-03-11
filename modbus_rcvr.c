/* Local headers */
#include "modbus_rcvr.h"
#include "uart.h"
/* Standard headers */
#include <stdio.h>
#include <stdlib.h> /* For EXIT_FAILURE & EXIT_SUCCESS */
#include <errno.h> /* for libmodbus */
#include <unistd.h> /* for write() */
#include <termios.h> /* For adjusting UART settings */
/* Non-standard headers */
#include <modbus.h>

/* Globals */
modbus_t *mb;
int uartfd;
char rand1, rand2;
char inputbuf[3];
int parityerr;
/* The zone1 command sender reads this mailbox and sends the appropriate */
/* command to the control system. */
int zone1_mailbox;

/*
 * Poll the modbus registers for zone1. The higher the address of the
 * register, the more important. It will override lower registers if both
 * are true at the same time.
 * Takes 1 parameter: a struct pollarg.
 * j counts down from the highest resgister given, to the lowest register
 * given. It goes backwards through the enum of rules and checks each one
 * to see if the modbus register is true or false. The modbus register
 * numbers are obtained from the ICA modbus server.
 */
void *poll_zone1(const void *args) {
  if(NULL == args) {
    fprintf(stderr, "zone1 poller given null args. Closing\n");
    return NULL;
  }
/* Needs 3 bytes for checking parity errors. */
  uint16_t rc, i, j;
  uint16_t *dest = malloc(sizeof(uint16_t));
  struct pollarg pa = *((struct pollarg *)(args));
  j = pa.high - pa.low;
  if(pa.high < pa.low) {
    fprintf(stderr, "Invalid high and low registers given to zone1 poller."\
		    " Closing\n");
    return NULL;
  }
  if((pa.high - pa.low) > NUM_RULES) {
    fprintf(stderr, "The number of rules to check is greater than the "\
                    "number of rules defined. See modbus_rcvr.h\n");
    return NULL;
  }
  while(1) {
    rand1 = (char)(48+rand()%5);
    rand2 = (char)(53+rand()%5);
    printf("rand1: %c, rand2: %c\n", rand1, rand2);
/* Reset the counter when j wraps around to 2^16-1(e.g 5,4,3,2,1,0,65536) */
    if(j > NUM_RULES) {
      j = pa.high - pa.low;
    }
/* Tell the slave (ICA) to read the jth holding register to dest. */
    if(-1 == (rc = modbus_read_registers(mb,j,1, dest))) {
      fprintf(stderr, "zone1 modbus_read_failed: %s\n", modbus_strerror(errno));
      return NULL;
    }
    if(FAN100 == j) {
      for(i = 0; i < rc; i++) {
        if(dest[i] != 0) {
          printf("FAN100: reg[%d]=%u (0x%X)\n", i, *dest, *dest);
/* Write the op-code for this rule to the uart */
          write(uartfd, Z1FAN100, sizeof(char));
/* Reset j so that we only transmit one rule at a time. */
          j = pa.high - pa.low;
	      }
      }
    }
    else if(FAN87_5 == j) {
      for(i = 0; i < rc; i++) {
        if(dest[i] != 0) {
          printf("FAN87_5: reg[%d]=%u (0x%X)\n", i, *dest, *dest);
          write(uartfd, Z1FAN87_5, sizeof(char));
          j = pa.high - pa.low;
	      }
      }
    }
    else if(FAN75 == j) {
      for(i = 0; i < rc; i++) {
        if(dest[i] != 0) {
          printf("FAN75: reg[%d]=%u (0x%X)\n", i, *dest, *dest);
          write(uartfd, Z1FAN75, sizeof(char));
          j = pa.high - pa.low;
	      }
      }
    }
    else if(FAN62_5 == j) {
      for(i = 0; i < rc; i++) {
        if(dest[i] != 0) {
          printf("FAN62_5: reg[%d]=%u (0x%X)\n", i, *dest, *dest);
          write(uartfd, Z1FAN62_5, sizeof(char));
          zone1_mailbox = FAN62_5;
          j = pa.high - pa.low;
	      }
      }
    }
    else if(FAN50 == j) {
      for(i = 0; i < rc; i++) {
        if(dest[i] != 0) {
          printf("FAN50: reg[%d]=%u (0x%X)\n", i, *dest, *dest);
          write(uartfd, Z1FAN50, sizeof(char));
          j = pa.high - pa.low;
	      }
      }
    }
    j--;
    sleep(RULE_CHK_FREQ);
  }/*while(1)*/
}

/*
 * Recieve modbus TCP from the ICA when tags are detected in a zone.
 * Appropriate commands are sent out through the 8Y1 UART to control
 * devices.
 */
int main() {
  uartfd = get_uartfd();
  struct termios term;
  speed_t speed; /* Store the baud rate */

/* The way termios works is by providing a struct termios with a set of */
/* flags. You can set the flags manually, or use the set and get */
/* functions. When you have set the flags the way you want, use */
/* tcsetattr to send the confinguration to the uart firmware. */
/* You can use tcgetattr to get the existing options from the uart. */
/* The safest way to set attributes is to get the existing ones first */
/* and then change only the flags you want by bitwise OR'ing them into */
/* each flag member of the struct termios. */

  tcgetattr(uartfd, &term);
/* Enable even parity checking */
  term.c_cflag |= PARENB;
/* If there is a parity error, append \377 (or (char)-1) twice and */
/* then add a null byte (\000). */
  term.c_iflag |= PARMRK;
/* Make sure the output baud rate is 9600 */
  speed = cfgetospeed(&term);
  if(speed != B9600) {
    cfsetospeed(&term, B9600);
  }
/* Push these new configurations to the UART driver immediately*/
  tcsetattr(uartfd, TCSANOW, &term);

  mb = modbus_new_tcp(MODBUS_SERV_IP, MODBUS_PORT);
  if(-1 == modbus_connect(mb)) {
    fprintf(stderr, "Connection failed: %s\n", modbus_strerror(errno));
    modbus_free(mb);
    return -1;
  }
  struct pollarg z1 = {4, 0};
  if(NULL == poll_zone1(&z1)) {
    return(EXIT_FAILURE);
  }
  /*TODO:
   * Set up pthreads.
   * Will also need semaphores for writing to uart.
   */
  modbus_close(mb);
  close(uartfd);
  modbus_free(mb);
  return 0;
}

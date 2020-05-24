/**************************************************************************
 * Authour : Ben Haubrich                                                 *
 * File    : fans.c                                                       *
 * Synopsis: fan control for EE/CME MST Capstone Design Project           *
 * ************************************************************************/
/* Local headers */
#include "fans.h"
#include "uart.h"
#include "doors.h"
/* Standard headers */
#include <stdio.h>
#include <stdlib.h> /* For EXIT_FAILURE & EXIT_SUCCESS */
#include <errno.h> /* for libmodbus */
#include <unistd.h> /* for write() */
#include <termios.h> /* For adjusting UART settings */
#include <pthread.h>
#include <poll.h>
/* Non-standard headers */
#include <modbus.h>

/* Globals */
modbus_t *mb_fan;
modbus_t *mb_door;
int uartfd;
//char rand1, rand2;
/* Needs 3 bytes for checking parity errors. */
char inputbuf[3];
/* The fans command sender reads this mailbox and sends the appropriate */
/* command to The control system. */
struct mailbox {
  int rule;
  int fan;
}fans_mailbox;

/*
 * Converts a rule to the corresponding command.
 * param fan
 *   The fan number (1 through 5)
 * param rule
 *   The rule to send
 * returns the command than can be sent through the UART.
 */
char *rule_to_cmd(int fan, int rule) {
  switch(fan) {
    case(1):
    if(FAN50 == rule) {
      return F1FAN50;
    }
    else if(FAN62_5 == rule) {
      return F1FAN62_5;
    }
    else if(FAN75 == rule) {
      return F1FAN75;
    }
    else if(FAN87_5 == rule) {
      return F1FAN87_5;
    }
    else {
      return F1FAN100;
    }
  }
  return NULL; /* temporary */
}

/*
 * Loop and send until the command has been received properly.
 */
void *fans_sendcmd(void) {
/* The current rule being enforced by ICA. */
  int currentrule;
/* The command to send. */
  char *cmd;
  char response = '\0';
  struct pollfd fds;
  fds.fd = uartfd;
  fds.events = POLLIN;
  while(1) {
    while((response != 'z' || fans_mailbox.rule != currentrule) &&
                                      fans_mailbox.rule <= NUM_FAN_RULES) {
      currentrule = fans_mailbox.rule;
      printf("currentrule %d\n", currentrule);
      cmd = rule_to_cmd(fans_mailbox.fan, currentrule);
      printf("Sending fan cmd: %s\n", cmd);
      write(uartfd, cmd, sizeof(char));
      usleep(10000);
      write(uartfd, cmd+1, sizeof(char));
      if(-1 == poll(&fds, 1, 1)) {
        perror("poll(): ");
      }
      if(POLLIN == fds.revents) {
        read(uartfd, &response, sizeof(char));
        printf("response: %c\n", response);
      }
    sleep(RULE_CHK_FREQ);
    }
  }
  return NULL;
}
  

/*
 * Poll the modbus registers for fans. The higher the address of the
 * register, the more important. It will override lower registers if both
 * are true at the same time.
 * Takes 1 parameter: a struct pollarg.
 * j counts down from the highest resgister given, to the lowest register
 * given. It goes backwards through the enum of rules and checks each one
 * to see if the modbus register is true or false. The modbus register
 * numbers are obtained from the ICA modbus server.
 * Zone1 includes:
 *   Fan1
 * Zone2 includes:
 *   Fan2
 * Zone3 includes:
 *   Fan3
 */
void *poll_fan1(const void *args) {
  if(NULL == args) {
    fprintf(stderr, "fans poller given null args. Closing\n");
    return NULL;
  }
  uint16_t rc, i, j;
  uint16_t *dest = malloc(sizeof(uint16_t));
  struct pollarg pa = *((struct pollarg *)(args));
  j = pa.high - pa.low;
  if(pa.high < pa.low) {
    fprintf(stderr, "Invalid high and low registers given to fans poller."\
		    " Closing\n");
    return NULL;
  }
  if((pa.high - pa.low) > NUM_FAN_RULES) {
    fprintf(stderr, "The number of rules to check is greater than the "\
                    "number of rules defined. See fans.h\n");
    return NULL;
  }
  while(1) {
/* TEMP: rand1 and rand2 are for testing. */
//    rand1 = (char)(48+rand()%5);
//    rand2 = (char)(53+rand()%5);
//    printf("rand1: %c, rand2: %c\n", rand1, rand2);
/* Reset the counter when j wraps around to 2^16-1(e.g 5,4,3,2,1,0,65536) */
    if(j > NUM_FAN_RULES) {
      j = pa.high - pa.low;
    }
/* Tell the slave (ICA) to read the jth holding register to dest. */
    if(-1 == (rc = modbus_read_registers(mb_fan,j,1, dest))) {
      fprintf(stderr, "fans modbus_read_failed: %s\n", modbus_strerror(errno));
      return NULL;
    }
    if(FAN100 == j) {
      for(i = 0; i < rc; i++) {
        if(dest[i] != 0) {
          printf("FAN100: reg[%d]=%u (0x%X)\n", i, *dest, *dest);
/* Write the op-code for this rule to the uart */
          write(uartfd, F1FAN100, sizeof(char));
/* Reset j so that we only transmit one rule at a time. */
          j = pa.high - pa.low;
	      }
      }
    }
    else if(FAN87_5 == j) {
      for(i = 0; i < rc; i++) {
        if(dest[i] != 0) {
          printf("FAN87_5: reg[%d]=%u (0x%X)\n", i, *dest, *dest);
          write(uartfd, F1FAN87_5, sizeof(char));
          j = pa.high - pa.low;
	      }
      }
    }
    else if(FAN75 == j) {
      for(i = 0; i < rc; i++) {
        if(dest[i] != 0) {
          printf("FAN75: reg[%d]=%u (0x%X)\n", i, *dest, *dest);
          //write(uartfd, F1FAN75, sizeof(char));
          fans_mailbox.rule = FAN75;
          fans_mailbox.fan = 1;
          j = pa.high - pa.low;
	      }
      }
    }
    else if(FAN62_5 == j) {
      for(i = 0; i < rc; i++) {
        if(dest[i] != 0) {
          printf("FAN62_5: reg[%d]=%u (0x%X)\n", i, *dest, *dest);
          //write(uartfd, F1FAN62_5, sizeof(char));
          fans_mailbox.rule = FAN62_5;
          fans_mailbox.fan = 1;
          j = pa.high - pa.low;
	      }
      }
    }
    else if(FAN50 == j) {
      for(i = 0; i < rc; i++) {
        if(dest[i] != 0) {
          printf("FAN50: reg[%d]=%u (0x%X)\n", i, *dest, *dest);
          //write(uartfd, F1FAN50, sizeof(char));
          fans_mailbox.rule = FAN50;
          fans_mailbox.fan = 1;
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

/* Set the mailbox to an invalid state to start off with so that we don't *//* transmit before we have received a rule. */
  fans_mailbox.rule = NUM_FAN_RULES+1;

/* The way termios works is by providing a struct termios with a set of */
/* flags. You can set the flags manually, or use the set and get */
/* functions. When you have set the flags the way you want, use */
/* tcsetattr to send the confinguration to the uart firmware. */
/* You can use tcgetattr to get the existing options from the uart. */
/* The safest way to set attributes is to get the existing ones first */
/* and then change only the flags you want by bitwise OR'ing them into */
/* each flag member of the struct termios. */
  tcgetattr(uartfd, &term);
/* Enable even parity checking. */
  term.c_cflag |= PARENB;
  term.c_cflag &= ~PARODD;
/* If there is a parity error, prefix \377 (or (char)-1 and a null byte */
/* (\000). */
  term.c_iflag |= PARMRK;
/* Make sure the output baud rate is 9600 */
  speed = cfgetospeed(&term);
  if(speed != B9600) {
    cfsetospeed(&term, B9600);
  }
/* Disable canonical mode so that special characters are not needed to */
/* read from the UART FIFO. */
  term.c_lflag &= ~ICANON;
/* Push these new configurations to the UART driver immediately*/
  tcsetattr(uartfd, TCSANOW, &term);

  mb_fan = modbus_new_tcp(MODBUS_SERV_IP, MODBUS_PORT);
  mb_door = modbus_new_tcp(MODBUS_SERV_IP, MODBUS_PORT);
  if(-1 == modbus_connect(mb_fan)) {
    fprintf(stderr, "Connection failed: %s\n", modbus_strerror(errno));
    modbus_free(mb_fan);
    return -1;
  }
  if(-1 == modbus_connect(mb_door)) {
    fprintf(stderr, "Connection failed: %s\n", modbus_strerror(errno));
    modbus_free(mb_door);
    return -1;
  }

  struct pollarg z1 = {4, 0};
/* Array of thread IDs */
  pthread_t tid[F1NUM_TASKS];
  void *tasks[F1NUM_TASKS] = {poll_fan1, fans_sendcmd, door_control};
  int retval;
  int i;
  for(i = 0; i < F1NUM_TASKS; i++) {
    retval = pthread_create(tid+i,
                            NULL, /* Default attributes */
                            tasks[i],
                            (void *)&z1); /* Pass the struct pollarg as */
                                         /* and argument. */
    if(-1 == retval) {
      perror("Could not start some tasks");
      exit(EXIT_FAILURE);
    }
  }
/* Stop the parent thread here and let the tasks run. If we don't do this *//* then the parent (main) will keep running, return 0, exit, and kill all *//* the threads. */
  for(i = 0; i < F1NUM_TASKS; i++) {
    retval = pthread_join(tid[i], NULL);
    if(-1 == retval) {
      perror("Error while joining threads");
    }
  }

  modbus_close(mb_fan);
  modbus_close(mb_door);
  close(uartfd);
  modbus_free(mb_fan);
  modbus_free(mb_door);
  return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> /* For access() */
/* For open() */
#include <sys/types.h> 
#include <sys/stat.h>
#include <fcntl.h> 

/*
 * Searches for avaialable uart character devices and returns a file
 * descriptor suitable for reading and writing to UART. Only returns a file
 * descriptor to a single device. Checks devices in order from ttyAMA[2:5].
 * returns the file descriptor on success, -1 otherwise.
 */
int get_uartfd() {
  char uartdev[13]; /* Path to uart device */
/* Check which UARTS are available. Does not check for UART0 because */
/* It's being used by the console */
  if(access("/dev/ttyAMA1", R_OK|W_OK) != -1) {
    strncpy(uartdev, "/dev/ttyAMA1", 13);
  }
  else if(access("/dev/ttyAMA2", R_OK|W_OK) != -1) {
    strncpy(uartdev, "/dev/ttyAMA2", 13);
  }
  else if(access("/dev/ttyAMA3", R_OK|W_OK) != -1) {
    strncpy(uartdev, "/dev/ttyAMA3", 13);
  }
  else if(access("/dev/ttyAMA4", R_OK|W_OK) != -1) {
    strncpy(uartdev, "/dev/ttyAMA4", 13);
  }
  else if(access("/dev/ttyAMA5", R_OK|W_OK) != -1) {
    strncpy(uartdev, "/dev/ttyAMA5", 13);
  }
  else {
    fprintf(stderr, "No UART device found\n");
    return -1;
  }
  return open(uartdev, O_RDWR);
}

/*
 * Check the received message for parity errors. PARMRK and PARENB must be
 * set in termios to enable this functionality.
 * returns -1 if a parity error occured, 0 otherwise.
 */
int check_parity_error(char msg[]) {
  if(msg[1] == '\377' && msg[2] == '\377') {
    return -1;
  }
  else {
    return 0;
  }
}

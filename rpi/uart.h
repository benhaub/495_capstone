/**************************************************************************
 * Authour : Ben Haubrich                                                 *
 * File    : uart.c                                                       *
 * Synopsis: uart control for raspberrypi 4                               *
 * ************************************************************************/

#ifndef __UART_H__
#define __UART_H__

/* To enable termios functionality */
#define _BSD_SOURCE /* For CBAUDEX */

int get_uartfd(void);
/* Test this by switching to odd parity and then back again. */
int check_parity_error(char[]);

#endif /*__UART_H__*/

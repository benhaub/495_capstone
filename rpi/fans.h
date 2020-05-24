/**************************************************************************
 * Authour : Ben Haubrich                                                 *
 * File    : fans.h                                                       *
 * Synopsis: fan control for EE/CME MST Capstone Design Project           *
 * ************************************************************************/
#ifndef __MODBUS_RCVR_H__
#define __MODBUS_RCVR_H__

#include <inttypes.h> /* for libmodbus */

/*
 * You can obtain the addresses that the zone uses from the ICA
 */
struct pollarg {
  int high; /* The highest address the zone uses for rules. */
  int low; /* The lowest address the zone uses for rules. */
};

/* Modbus holding register address */
#define ADDRESS 100
/* Modbus TCP server address */
#define MODBUS_SERV_IP "172.16.1.5"
/* Samba server port for modbus tcp */
#define MODBUS_PORT 11501

/* Acknowledge. Message correctly received */
#define ACK "z"

/* Fan Rules. Treat underscores as decimal points. */
enum fans_rules {FAN50, FAN62_5, FAN75, FAN87_5, FAN100};
#define NUM_FAN_RULES 4
/* Define how often we check for rules in seconds */
#define RULE_CHK_FREQ 1
/* Fan command codes. Defined from software running on separate hardware */
#define F1FAN50 "55"
#define F1FAN62_5 "56"
#define F1FAN75 "57"
#define F1FAN87_5 "59"
#define F1FAN100 "5A"
#define F2FAN50 "65"
#define F2FAN62_5 "66"
#define F2FAN75 "67"
#define F2FAN87_5 "69"
#define F2FAN100 "6A"
#define F3FAN50 "75"
#define F3FAN62_5 "76"
#define F3FAN75 "77"
#define F3FAN87_5 "79"
#define F3FAN100 "7A"
#define F4FAN50 "85"
#define F4FAN62_5 "86"
#define F4FAN75 "87"
#define F4FAN87_5 "89"
#define F4FAN100 "8A"
#define F5FAN50 "95"
#define F5FAN62_5 "96"
#define F5FAN75 "97"
#define F5FAN87_5 "99"
#define F5FAN100 "9A"
/* Number of zone 1 tasks for multithreading. */
#define F1NUM_TASKS 3

#endif /*MODBUS_RCVR_H__*/

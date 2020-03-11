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

/* Zone 1 Rules. Treat underscores as decimal points. */
enum zone1_rules {FAN50, FAN62_5, FAN75, FAN87_5, FAN100};
#define NUM_RULES 4
/* Define how often we check for rules in seconds */
#define RULE_CHK_FREQ 1
/* Zone 1 command codes */
#define Z1FAN50 "0"
#define Z1FAN62_5 "a"
#define Z1FAN75 "2"
#define Z1FAN87_5 "3"
#define Z1FAN100 "4"

#endif /*MODBUS_RCVR_H__*/

#ifndef __DOORS_H__

/* Door Rules */
enum door_rules {DOOR1 = 5, DOOR2 = 6, DOOR3 = 7, DOOR4 = 8};
#define NUM_DOORS 4
/* Door commands */
#define D1OPEN "10"
#define D1CLOSE "15"
#define D2OPEN "20"
#define D2CLOSE "25"
#define D3OPEN "30"
#define D3CLOSE "35"
#define D4OPEN "40"
#define D4CLOSE "45"

/* Prototypes */
void *door_control(void);

#endif /*__DOORS_H__*/

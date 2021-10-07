#ifndef DEFINITIONS_H_
#define DEFINITIONS_H_

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

// COMMANDS
#define CMD_PUSH_NOTIFICATION 1
#define CMD_SET_SERVER_CONFIGS 2
#define CMD_SET_DEVICE_VALUE 3
#define CMD_GET_TEMP_HUMD_DATA 4
#define CMD_GET_DEVICE_DATA 5

// RESPONSE STATUSES
#define OK_STATUS_RESPONSE 0
#define NOK_STATUS_RESPONSE -1

// DEVICE TYPES
#define PRESENCE_SENSOR 1
#define WINDOW_SENSOR 2
#define DOOR_SENSOR 3
#define COUNT_SENSOR 4
#define SMOKE_SENSOR 5
#define LAMP_OUT 6
#define AIR_CONDT_OUT 7
#define SPRINKLER_OUT 8

#endif /* DEFINITIONS_H_ */

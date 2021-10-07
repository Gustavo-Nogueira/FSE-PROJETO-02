#ifndef DEFINITIONS_H_
#define DEFINITIONS_H_

#define MAX_DEVICES 100  // maximum number of IO devices

typedef struct {
    int type;
    int gpio;
    char tag[200];
} DEVICE_CONFIG;

typedef struct {
    char ip[20];
    int port;
} ADDRESS;

typedef struct {
    int server_id;
    char name[100];
    ADDRESS address;         // my address
    ADDRESS central_server;  // central server address
    int size_inputs;
    int size_outputs;
    DEVICE_CONFIG inputs[MAX_DEVICES];
    DEVICE_CONFIG outputs[MAX_DEVICES];
} SERVER_CONFIG;

// DELTA TIME OF INTERRUPTIONS (usec)
#define MIN_DT_COUNT_SENSOR 100000
#define MAX_DT_COUNT_SENSOR 300000

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

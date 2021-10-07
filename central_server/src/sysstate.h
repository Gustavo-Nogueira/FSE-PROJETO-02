#ifndef SYSSTATE_H_
#define SYSSTATE_H_

#define N_DISTR_SERVERS 2  // number of servers distributed
#define MAX_DEVICES 100    // maximum number of IO devices

// STRUCTS

typedef struct {
    int type;
    int gpio;
    char tag[200];
    int current_value;
} DEVICE;

typedef struct {
    char ip[20];
    int port;
} ADDRESS;

typedef struct {
    int server_id;
    char name[100];
    ADDRESS address;
    int size_inputs;
    int size_outputs;
    DEVICE inputs[MAX_DEVICES];
    DEVICE outputs[MAX_DEVICES];
} DISTRIBUTED_SERVER;

typedef struct {
    int alarm;  // 1 for on and 0 for off
    int occupation;
    float humidity;
    float temperature;
    int current_view;  // distributed server being presented
    DISTRIBUTED_SERVER distr_servers[N_DISTR_SERVERS];
} SYSTEM_STATE;

// FUNCTIONS

// Getters
SYSTEM_STATE *get_system_state();

DISTRIBUTED_SERVER *get_distributed_server(int index);

// Setters
void set_system_state(SYSTEM_STATE *st);

void set_distr_server(int index, DISTRIBUTED_SERVER *server);

void set_device_value(int server_id, int gpio, int value);

void set_temperature(float value);

void set_humidity(float value);

void set_occupation(int value);

void set_current_view(int value);

void set_alarm(int value);

#endif /* SYSSTATE_H_ */

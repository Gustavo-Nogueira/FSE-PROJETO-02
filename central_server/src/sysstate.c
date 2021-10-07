#include "sysstate.h"

#include <stdlib.h>
#include <string.h>

SYSTEM_STATE _system_state;

// Getters
SYSTEM_STATE *get_system_state() {
    return &_system_state;
}

DISTRIBUTED_SERVER *get_distributed_server(int index) {
    return &_system_state.distr_servers[index];
}

// Setters
void set_system_state(SYSTEM_STATE *st) {
    memcpy(&_system_state, st, sizeof(SYSTEM_STATE));
}

void set_distr_server(int index, DISTRIBUTED_SERVER *server) {
    memcpy(&_system_state.distr_servers[index], server, sizeof(SYSTEM_STATE));
}

void set_device_value(int server_id, int gpio, int value) {
    int index;

    for (int i = 0; i < N_DISTR_SERVERS; i++) {
        if (_system_state.distr_servers[i].server_id == server_id) {
            index = i;
            break;
        }
    }

    int size = _system_state.distr_servers[index].size_inputs;
    for (int i = 0; i < size; i++) {
        if (_system_state.distr_servers[index].inputs[i].gpio == gpio) {
            _system_state.distr_servers[index].inputs[i].current_value = value;
            return;
        }
    }

    size = _system_state.distr_servers[index].size_outputs;
    for (int i = 0; i < size; i++) {
        if (_system_state.distr_servers[index].outputs[i].gpio == gpio) {
            _system_state.distr_servers[index].outputs[i].current_value = value;
            return;
        }
    }
}

void set_temperature(float value) {
    _system_state.temperature = value;
}

void set_humidity(float value) {
    _system_state.humidity = value;
}

void set_occupation(int value) {
    _system_state.occupation = value;
}

void set_current_view(int value) {
    _system_state.current_view = value;
}

void set_alarm(int value) {
    _system_state.alarm = value;
}
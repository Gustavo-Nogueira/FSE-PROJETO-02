#include <stdio.h>

#include "sysstate.h"

SYSTEM_STATE initial_sys_state = {0};

int main() {
    return 0;
}

/* 
int main() {
    printf("1=> %s | %d \n", initial_sys_state.distr_servers[0].name, initial_sys_state.distr_servers[0].size_inputs);
    printf("2=> %s | %d \n", initial_sys_state.distr_servers[0].inputs[0].tag, initial_sys_state.distr_servers[0].inputs[0].gpio);
    printf("3=> %s | %d \n", initial_sys_state.distr_servers[1].inputs[0].tag, initial_sys_state.distr_servers[1].inputs[0].gpio);
    return 0;
}

*/
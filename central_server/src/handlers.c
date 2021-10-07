#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

#include "sockets.h"

void *handle_request_output_value(void *arg) {}

void *handle_menu_window(void *args) {}

void *handle_system_state_window(void *args) {}

void handle_server_config() {}

void handle_push_notification() {}

void handle_request(char *json_request, char *json_response) {}

void *listen_requests_adapter(void *arg) {}
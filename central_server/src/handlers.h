#ifndef HANDLERS_H_
#define HANDLERS_H_

void *handle_request_output_value(void *arg);

void *handle_menu_window(void *arg);

void *handle_system_state_window(void *arg);

void *handle_menu_window(void *args);

void *handle_system_state_window(void *args);

void handle_server_config();

void handle_push_notification();

void handle_request(char *json_request, char *json_response);

void *listen_requests_adapter(void *arg);

#endif /* HANDLERS_H_ */

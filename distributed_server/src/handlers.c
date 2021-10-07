#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#include "cJSON.h"
#include "definitions.h"
#include "dht22.h"
#include "gpio.h"
#include "setup.h"
#include "sockets.h"

int handle_set_device_value(int gpio, int value) {
    if (write_gpio(gpio, value) < 0) {
        return NOK_STATUS_RESPONSE;
    }

    return OK_STATUS_RESPONSE;
}

int handle_get_temp_humd_data(cJSON *cj_data) {
    cJSON *cj_temperature = NULL, *cj_humidity = NULL;
    float temp_cels = -1, temp_fahr = -1, humidity = -1;

    get_dht_data(&temp_cels, &temp_fahr, &humidity);

    cj_temperature = cJSON_CreateNumber(temp_cels);
    cj_humidity = cJSON_CreateNumber(humidity);

    cj_data = cJSON_CreateObject();
    cJSON_AddItemToObject(cj_data, "temperature", cj_temperature);
    cJSON_AddItemToObject(cj_data, "humidity", cj_humidity);

    if (temp_cels < 0 || humidity < 0) {
        return NOK_STATUS_RESPONSE;
    }

    return OK_STATUS_RESPONSE;
}

int handle_get_device_data(cJSON *cj_data) {
    SERVER_CONFIG *sconfig = get_server_config();
    cJSON *cj_outputs = NULL, *cj_output = NULL, *cj_gpio = NULL, *cj_value = NULL;

    cj_outputs = cJSON_CreateArray();
    for (int i = 0; i < sconfig->size_outputs; i++) {
        cj_output = cJSON_CreateObject();
        cj_gpio = cJSON_CreateNumber(sconfig->outputs[i].gpio);
        cj_value = cJSON_CreateNumber(read_gpio(sconfig->outputs[i].gpio));
        cJSON_AddItemToObject(cj_output, "gpio", cj_gpio);
        cJSON_AddItemToObject(cj_output, "value", cj_value);
        cJSON_AddItemToArray(cj_outputs, cj_output);
    }

    cj_data = cJSON_CreateObject();
    cJSON_AddItemToObject(cj_data, "outputs", cj_outputs);

    free(sconfig);

    return OK_STATUS_RESPONSE;
}

void handle_request(char *json_request, char *json_response) {
    int command, gpio, value, status;
    cJSON *cj_command = NULL, *cj_data = NULL, *cj_status = NULL;
    cJSON *cj_request = cJSON_Parse(json_request);
    cJSON *cj_response = cJSON_CreateObject();
    cj_command = cJSON_GetObjectItemCaseSensitive(cj_request, "command");

    command = cj_command->valuedouble;

    printf("Requisicao Recebida [COMMAND: %d]\n", command);

    switch (command) {
        case CMD_SET_DEVICE_VALUE:
            cj_data = cJSON_GetObjectItemCaseSensitive(cj_request, "data");
            gpio = cJSON_GetObjectItemCaseSensitive(cj_data, "gpio")->valuedouble;
            value = cJSON_GetObjectItemCaseSensitive(cj_data, "value")->valuedouble;
            status = handle_set_device_value(gpio, value);
            cj_status = cJSON_CreateNumber(status);
            cJSON_AddItemToObject(cj_response, "status", cj_status);
            break;
        case CMD_GET_TEMP_HUMD_DATA:
            status = handle_get_temp_humd_data(cj_data);
            cj_status = cJSON_CreateNumber(status);
            cJSON_AddItemToObject(cj_response, "data", cj_data);
            cJSON_AddItemToObject(cj_response, "status", cj_status);
            break;
        case CMD_GET_DEVICE_DATA:
            status = handle_get_device_data(cj_data);
            cj_status = cJSON_CreateNumber(status);
            cJSON_AddItemToObject(cj_response, "data", cj_data);
            cJSON_AddItemToObject(cj_response, "status", cj_status);
            break;
        default:
            break;
    }

    json_response = cJSON_Print(cj_response);

    cJSON_Delete(cj_request);
    cJSON_Delete(cj_response);
}

void *listen_requests_adapter(void *arg) {
    int socket_fd = *((int *)arg);

    listen_requests(socket_fd, &handle_request);

    return NULL;  // skip warning
}

void handle_interruption(int gpio, int value, unsigned long delta_time) {
    char *json_request;
    int device_type;
    struct sockaddr_in addr;
    SERVER_CONFIG *sconfig = get_server_config();
    cJSON *cj_request = NULL, *cj_data = NULL, *cj_item = NULL;

    for (int i = 0; i < sconfig->size_inputs; i++) {
        if (sconfig->inputs[i].gpio == gpio) {
            device_type = sconfig->inputs[i].type;
            break;
        }
    }

    // Filter jitter
    switch (device_type) {
        case COUNT_SENSOR:
            if (delta_time < MIN_DT_COUNT_SENSOR || delta_time > MAX_DT_COUNT_SENSOR) {
                return;
            }
            break;
        default:
            break;
    }

    printf("Interrupcao Detectada [GPIO: %2d] => [VALOR: %d]\n", gpio, value);

    create_sockaddr(sconfig->central_server.ip, sconfig->central_server.port, &addr);

    cj_data = cJSON_CreateObject();
    cj_request = cJSON_CreateObject();

    cj_item = cJSON_CreateNumber(CMD_PUSH_NOTIFICATION);
    cJSON_AddItemToObject(cj_request, "command", cj_item);

    cj_item = cJSON_CreateNumber(sconfig->server_id);
    cJSON_AddItemToObject(cj_request, "server_id", cj_item);

    cj_item = cJSON_CreateNumber(gpio);
    cJSON_AddItemToObject(cj_data, "gpio", cj_item);

    cj_item = cJSON_CreateNumber(value);
    cJSON_AddItemToObject(cj_data, "value", cj_item);

    cJSON_AddItemToObject(cj_request, "data", cj_data);

    json_request = cJSON_Print(cj_request);

    send_request(json_request, &addr, NULL);

    free(json_request);
    cJSON_Delete(cj_request);
}

void handle_sigint(int sig) {
    signal(sig, handle_sigint);

    printf("Encerrando...\n");

    close_gpios();

    close_server_socket();
}
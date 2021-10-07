#include "setup.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "cJSON.h"

SERVER_CONFIG _server_config;

static void set_address(cJSON *cjson_addr, ADDRESS *addr) {
    cJSON *item;

    item = cJSON_GetObjectItemCaseSensitive(cjson_addr, "ip");
    if (cJSON_IsString(item) && (item->valuestring != NULL)) {
        memcpy(addr->ip, item->valuestring, strlen(item->valuestring));
    }

    item = cJSON_GetObjectItemCaseSensitive(cjson_addr, "port");
    if (cJSON_IsNumber(item)) {
        addr->port = item->valueint;
    }
}

static int set_devices(cJSON *cjson_devices, DEVICE_CONFIG *dconfigs) {
    int i = 0;
    cJSON *device, *item;

    cJSON_ArrayForEach(device, cjson_devices) {
        if (i < MAX_DEVICES) {
            item = cJSON_GetObjectItemCaseSensitive(device, "tag");
            if (cJSON_IsString(item) && (item->valuestring != NULL)) {
                memcpy(dconfigs[i].tag, item->valuestring, strlen(item->valuestring));
            }
            item = cJSON_GetObjectItemCaseSensitive(device, "type");
            if (cJSON_IsNumber(item)) {
                dconfigs[i].type = item->valueint;
            }
            item = cJSON_GetObjectItemCaseSensitive(device, "gpio");
            if (cJSON_IsNumber(item)) {
                dconfigs[i].gpio = item->valueint;
            }
            i++;
        }
    }

    return i;
}

int set_server_config(char *json_config) {
    cJSON *item;
    cJSON *root = cJSON_Parse(json_config);
    SERVER_CONFIG *sconfig = &_server_config;

    if (root == NULL) {
        return -1;
    }

    item = cJSON_GetObjectItemCaseSensitive(root, "server_id");
    if (cJSON_IsNumber(item)) {
        sconfig->server_id = item->valueint;
    }

    item = cJSON_GetObjectItemCaseSensitive(root, "name");
    if (cJSON_IsString(item) && (item->valuestring != NULL)) {
        memcpy(sconfig->name, item->valuestring, strlen(item->valuestring));
    }

    item = cJSON_GetObjectItemCaseSensitive(root, "address");
    set_address(item, &sconfig->address);

    item = cJSON_GetObjectItemCaseSensitive(root, "central_server");
    set_address(item, &sconfig->central_server);

    item = cJSON_GetObjectItemCaseSensitive(root, "outputs");
    sconfig->size_outputs = set_devices(item, sconfig->outputs);

    item = cJSON_GetObjectItemCaseSensitive(root, "inputs");
    sconfig->size_inputs = set_devices(item, sconfig->inputs);

    cJSON_Delete(root);

    return 0;
}

SERVER_CONFIG *get_server_config() {
    SERVER_CONFIG *config = malloc(sizeof(SERVER_CONFIG));
    memcpy(config, &_server_config, sizeof(SERVER_CONFIG));
    return config;
}

int read_server_config(char *config_file, char **json_config) {
    FILE *fp;
    int len;

    fp = fopen(config_file, "rb");

    if (fp == NULL) {
        return -1;
    }

    fseek(fp, 0, SEEK_END);
    len = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    *json_config = (char *)malloc(len + 1);
    fread(*json_config, 1, len, fp);

    fclose(fp);

    return 0;
}

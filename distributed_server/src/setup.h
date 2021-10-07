#ifndef SETUP_H_
#define SETUP_H_

#include "definitions.h"

SERVER_CONFIG *get_server_config();

int set_server_config(char *json_config);

int read_server_config(char *config_file, char **json_config);

#endif /* SETUP_H_ */

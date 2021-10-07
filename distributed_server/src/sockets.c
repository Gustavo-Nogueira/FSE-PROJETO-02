/**
 * TCP sockets based communication module.
*/

#include "sockets.h"

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

static int _server_socket;

int init_server_socket(struct sockaddr_in *addr) {
    // create socket tcp
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (socket_fd < 0) {
        return -1;
    }

    if (bind(socket_fd, (struct sockaddr *)addr, sizeof(*addr)) < 0) {
        return -2;
    }

    if (listen(socket_fd, MAX_QUEUE_TCP) < 0) {
        return -3;
    }

    _server_socket = socket_fd;

    return socket_fd;
}

void close_server_socket() {
    close(_server_socket);
}

void create_sockaddr(char *ip, int port, struct sockaddr_in *addr) {
    addr->sin_family = AF_INET;
    addr->sin_addr.s_addr = inet_addr(ip);
    addr->sin_port = htons(port);
}

int send_request(char *json_request, struct sockaddr_in *dest, RESPONSE_HANDLER handler) {
    char json_response[MAX_JSON_SZ];

    // create socket tcp
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (socket_fd < 0) {
        return -1;
    }

    if (connect(socket_fd, (struct sockaddr *)dest, sizeof(*dest)) < 0) {
        return -2;
    }

    if (send(socket_fd, &json_request, strlen(json_request), 0) < 0) {
        return -3;
    }

    if (recv(socket_fd, &json_response, sizeof(json_response), 0) < 0) {
        return -4;
    }

    if (handler != NULL) {
        handler(json_response);
    }

    close(socket_fd);

    return 0;
}

void listen_requests(int socket_fd, REQUEST_HANDLER handler) {
    char json_request[MAX_JSON_SZ], json_response[MAX_JSON_SZ];
    struct sockaddr_in addr_client;
    int socket_client;
    unsigned int addr_client_sz = sizeof(addr_client);

    for (;;) {
        socket_client = accept(socket_fd, (struct sockaddr *)&addr_client, &addr_client_sz);

        if (socket_client < 0) {
            continue;
        }

        if (recv(socket_client, &json_request, sizeof(json_request), 0) < 0) {
            close(socket_client);
            continue;
        }

        handler(json_response, json_response);

        if (send(socket_client, &json_response, strlen(json_response), 0) < 0) {
            close(socket_client);
            continue;
        }

        close(socket_client);
    }
}

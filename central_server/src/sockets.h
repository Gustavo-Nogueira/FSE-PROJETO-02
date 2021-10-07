#ifndef SOCKETS_H_
#define SOCKETS_H_

#include <netinet/in.h>

#define MAX_JSON_SZ 1000000
#define MAX_QUEUE_TCP 5

typedef void (*RESPONSE_HANDLER)(char *json_response);

typedef void (*REQUEST_HANDLER)(char *json_request, char *json_response);

int init_server_socket(struct sockaddr_in *addr);

void close_server_socket();

void create_sockaddr(char *ip, int port, struct sockaddr_in *addr);

int send_request(char *json_request, struct sockaddr_in *dest, RESPONSE_HANDLER handler);

void listen_requests(int socket_fd, REQUEST_HANDLER handler);

#endif /* SOCKETS_H_ */

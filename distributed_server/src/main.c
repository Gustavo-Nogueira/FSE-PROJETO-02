#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "gpio.h"
#include "handlers.h"
#include "setup.h"
#include "sockets.h"

int main(int argc, char *argv[]) {
    signal(SIGINT, handle_sigint);

    int server_socket;
    char *json_config = NULL;
    SERVER_CONFIG *sconfig = NULL;
    struct sockaddr_in central_addr, server_addr;

    if (argc != 2) {
        printf("Insira a path do arquivo de configuracao.\n");
        exit(EXIT_FAILURE);
    }

    printf("Carregando arquivo de configuracao... ");
    if (read_server_config(argv[1], &json_config) < 0) {
        printf("FALHOU!\n");
        return EXIT_FAILURE;
    }
    if (set_server_config(json_config) < 0) {
        printf("FALHOU!\n");
        return EXIT_FAILURE;
    }
    printf("OK!\n");

    sconfig = get_server_config();

    /*     printf("Estabelecendo conexao com o servidor central... ");
    create_sockaddr(sconfig->central_server.ip, sconfig->central_server.port, &central_addr);
    if (send_request(json_config, &central_addr, NULL) < 0) {
        printf("FALHOU!\n");
        return EXIT_FAILURE;
    }
    printf("OK!\n"); */

    printf("Inicializando GPIOs... \n");
    if (init_gpios(sconfig, &handle_interruption) < 0) {
        printf("FALHOU!\n");
        return EXIT_FAILURE;
    }
    printf("OK! Inicializacao finalizada.\n");

    printf("Inicializando socket do servidor distribuido... ");
    create_sockaddr(sconfig->address.ip, sconfig->address.port, &server_addr);
    if ((server_socket = init_server_socket(&server_addr)) < 0) {
        printf("FALHOU!\n");
        printf("Codigo de erro: %d\n", server_socket);
        return EXIT_FAILURE;
    }

    pthread_t server_tid;
    pthread_create(&server_tid, NULL, &listen_requests_adapter, (void *)&server_socket);
    printf("OK!\n");
    printf("Pronto para receber requisicoes.\n");

    pause();

    return 0;
}

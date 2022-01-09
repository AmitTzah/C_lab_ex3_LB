#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_LEN_RECV 1000
#define MIN_PORT_VAL 1024
#define MAX_PORT_VAL 64000
#define LISTEN_BACKLOG 10
#define NUMBER_OF_SERVERS 3
#define LOCAL_HOST "127.0.0.1"
#define SIZE_OF_REQUEST_END 4
#define handle_error(msg) \
  do {                    \
    perror(msg);          \
    exit(EXIT_FAILURE);   \
  } while (0)

const char *REQUEST_END = "\r\n\r\n";

int ranged_rand(int lower, int upper) { return (rand() % (upper - lower + 1)) + lower; }

void bind_and_print_port(const int *socket, struct sockaddr_in service, int *port, FILE *fp)
{
    while (bind(*socket, (struct sockaddr *)&service, sizeof(service)) == -1) {
        *port = ranged_rand(MIN_PORT_VAL, MAX_PORT_VAL);
        service.sin_port = htons(*port);
    }
    fprintf(fp, "%d", *port);
    fclose(fp);
    if (listen(*socket, LISTEN_BACKLOG) == -1) {
        handle_error("listen");
    }
}

struct sockaddr_in set_sockaddr_parameters(const int *port)
{
    struct sockaddr_in service;
    service.sin_family = AF_INET;
    service.sin_addr.s_addr = inet_addr(LOCAL_HOST);
    service.sin_port = htons(*port);
    return service;
}

void connect_all_servers(int *servers_array, const int *main_server_socket, struct sockaddr_in service, int *addr_len){
    int i = 0;
    while (i < NUMBER_OF_SERVERS) {  // Waiting till all servers are connected
        servers_array[i] = accept(*main_server_socket, (struct sockaddr *)&service, addr_len);
        if (servers_array[i] != -1) {
            i++;
            listen(*main_server_socket, LISTEN_BACKLOG);
        } else {
            handle_error("accept");
        }
    }
}

int return_number_of_times_request_end_in_request(char *request, size_t size_of_full_request)
{
    int count_request_end = 0;
    size_t j;

    for (j = 0; j < (size_of_full_request + 1); j++) {
        if (strncmp(&(request[j]), REQUEST_END, SIZE_OF_REQUEST_END) ==0){
            count_request_end++;
        }
    }
    return count_request_end;
}

void receives_message_to_full_request_until_num_of_request_ends(int number_of_requests_ends, int accept_client_socket,
                                                                char **full_request, size_t *size_of_full_request)
{
    char receive_buffer[MAX_LEN_RECV];
    do {
        size_t bytes_recv = recv(accept_client_socket, receive_buffer, MAX_LEN_RECV, 0);
        size_t old_size_of_full_request = *size_of_full_request;
        *size_of_full_request += bytes_recv;
        *full_request = (char *)realloc(*full_request, *size_of_full_request);
        char *pointer_to_start_of_current_message = *full_request + old_size_of_full_request;
        memcpy((pointer_to_start_of_current_message), receive_buffer, bytes_recv);
    } while (return_number_of_times_request_end_in_request(*full_request, *size_of_full_request) !=
             number_of_requests_ends);
}

void LB_loop(int *accept_client_socket, const int *main_client_socket, struct sockaddr_in service_client, int *addr_len,
                size_t *size_of_full_request, int *servers_accept_sockets_array,  char **full_request){
    int i = 0;
    while (1) {
        *accept_client_socket = accept(*main_client_socket, (struct sockaddr *)&service_client, addr_len);
        while (*accept_client_socket < 0) {  // failed to accept, try again
            listen(*main_client_socket, LISTEN_BACKLOG);
            *accept_client_socket = accept(*main_client_socket, (struct sockaddr *)&service_client, addr_len);
        }
        receives_message_to_full_request_until_num_of_request_ends(1, *accept_client_socket, full_request,
                                                                   size_of_full_request);

        send(servers_accept_sockets_array[i], *full_request, *size_of_full_request, 0);  // sends request to current server
        *size_of_full_request = 0;
        free(*full_request);
        *full_request = malloc(sizeof(char));
        receives_message_to_full_request_until_num_of_request_ends(2, servers_accept_sockets_array[i], full_request,
                                                                   size_of_full_request);

        send(*accept_client_socket, *full_request, *size_of_full_request, 0);
        *size_of_full_request = 0;
        i = (i == NUMBER_OF_SERVERS - 1) ? 0 : (i + 1);
    }
}

int main()
{
    int port_server, port_client, servers_accept_sockets_array[NUMBER_OF_SERVERS], accept_client_socket;
    int addr_len = sizeof(struct sockaddr_in);
    size_t size_of_full_request = 0;
    char *full_request = malloc(sizeof(char));
    FILE *server_port_number, *client_port_number;
    server_port_number = fopen("server_port", "w");
    client_port_number = fopen("http_port", "w");
    int main_client_socket = socket(AF_INET, SOCK_STREAM, 0);
    int main_server_socket = socket(AF_INET, SOCK_STREAM, 0);

    if (main_client_socket == -1 || main_server_socket == -1) {
        handle_error("socket");
    }

    srand(time(NULL));
    port_server = ranged_rand(MIN_PORT_VAL, MAX_PORT_VAL);
    port_client = ranged_rand(MIN_PORT_VAL, MAX_PORT_VAL);

    struct sockaddr_in service_client, service_server;
    service_client = set_sockaddr_parameters(&port_client);
    service_server = set_sockaddr_parameters(&port_server);
    bind_and_print_port(&main_server_socket, service_server, &port_server, server_port_number);
    bind_and_print_port(&main_client_socket, service_server, &port_client, client_port_number);

    connect_all_servers(servers_accept_sockets_array, &main_server_socket, service_server, &addr_len);

    LB_loop(&accept_client_socket, &main_client_socket, service_client, &addr_len, &size_of_full_request,
            servers_accept_sockets_array,  &full_request);
}
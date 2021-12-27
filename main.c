#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>


#define MAX_LEN_RECV 1000 // I'm not sure what should we use
#define MIN_PORT_VAL 1024
#define MAX_PORT_VAL 64000
#define LISTEN_BACKLOG 10
#define NUMBER_OF_SERVERS 3
#define LOCAL_HOST "127.0.0.1"


#define handle_error(msg) \
           do { perror(msg); exit(EXIT_FAILURE); } while (0)


int ranged_rand(int lower, int upper)
{
    return (rand() % (upper - lower + 1)) + lower;
}


int  return_number_of_times_request_end_in_request(char* request, size_t size_of_full_request) {
    int count_request_end=0;
    size_t j;
    if(size_of_full_request<4){

        return 0;
    }

    for(j=4; j<(size_of_full_request+1);j++) {
        if ((request[j - 4] == '\r') && (request[j - 3] == '\n') && (request[j - 2] == '\r') &&
            (request[j - 1] == '\n')) {
            count_request_end++;

        }
    }
    return count_request_end;
}

void receives_message_to_full_request_until_num_of_request_ends(int number_of_requests_ends, int accept_client_socket
                                                                ,char** full_request, size_t* size_of_full_request){
    char receive_buffer[MAX_LEN_RECV];

    do {
        size_t  bytes_recv= recv(accept_client_socket, receive_buffer, MAX_LEN_RECV, 0);
        size_t old_size_of_full_request=*size_of_full_request;
        *size_of_full_request+=bytes_recv;
        printf("%s\n",receive_buffer);
        *full_request= (char *) realloc(*full_request, *size_of_full_request);

        char* pointer_to_start_of_current_message=*full_request+old_size_of_full_request;

        memcpy((pointer_to_start_of_current_message),receive_buffer, bytes_recv);

        printf("%s\n",*full_request);

    }while (return_number_of_times_request_end_in_request(*full_request, *size_of_full_request) != number_of_requests_ends);
}

int main() {

    int port_server, port_client, servers_accept_sockets_array[3], accept_client_socket;
    int addr_len = sizeof(struct sockaddr_in);
    size_t size_of_full_request=0;

    char* full_request= malloc(sizeof(char));


    FILE *server_port_number, *client_port_number;
    server_port_number = fopen("server_port.txt", "w");
    client_port_number = fopen("http_port.txt", "w");

    //create a new socket for servers and for client
    int main_client_socket = socket(AF_INET, SOCK_STREAM, 0);
    int main_server_socket = socket(AF_INET, SOCK_STREAM, 0);

    if (main_client_socket == -1 || main_server_socket == -1){
        handle_error("socket");}


    //create sockaddr variable with random porn number
    srand ( time(NULL) );
    port_server = ranged_rand(MIN_PORT_VAL, MAX_PORT_VAL);
    port_client = ranged_rand(MIN_PORT_VAL, MAX_PORT_VAL);

    struct sockaddr_in service_client, service_server;
    service_client.sin_family = AF_INET;
    service_client.sin_addr.s_addr = inet_addr( LOCAL_HOST );
    service_client.sin_port = htons(port_client);

    service_server.sin_family = AF_INET;
    service_server.sin_addr.s_addr = inet_addr( LOCAL_HOST );
    service_server.sin_port = htons(port_server);

    //bind sever socket, keep trying until success
    while (bind(main_server_socket, (struct sockaddr*)&service_server, sizeof(service_server)) == -1) {
        port_server = ranged_rand(MIN_PORT_VAL, MAX_PORT_VAL);
        service_server.sin_port = htons(port_server);
    }
    fprintf(server_port_number, "%d", port_server);
    fclose(server_port_number);
    if (listen(main_server_socket, LISTEN_BACKLOG) == -1)
        handle_error("listen");


    //bind client socket, keep trying until success
    while (bind(main_client_socket, (struct sockaddr*)&service_client, sizeof(service_client)) == -1) {
        port_client = ranged_rand(MIN_PORT_VAL, MAX_PORT_VAL);
        service_client.sin_port = htons(port_client);
    }
    fprintf(client_port_number, "%d", port_client);
    fclose(client_port_number);
    if (listen(main_client_socket, LISTEN_BACKLOG) == -1)
        handle_error("listen");

    // Waiting till 3 servers are connected
    int i = 0;
    while (i<NUMBER_OF_SERVERS){
        servers_accept_sockets_array[i] = accept(main_server_socket, (struct sockaddr*)&service_server, &addr_len);
        if(servers_accept_sockets_array[i] != -1 ) {
            i++;
            listen(main_server_socket, LISTEN_BACKLOG);
        }
        else{
            handle_error("accept");

        }
    }


    i = 0;
    while (1){
        // Accepting http connection
        accept_client_socket = accept(main_client_socket, (struct sockaddr *) &service_client, &addr_len);
        while(accept_client_socket < 0) { // failed to accept, try again
            listen(main_client_socket, LISTEN_BACKLOG);
            accept_client_socket = accept(main_client_socket, (struct sockaddr *) &service_client, &addr_len);
        }

        receives_message_to_full_request_until_num_of_request_ends(1, accept_client_socket
        ,&full_request, &size_of_full_request);

        //sends the string to next server by order
        send(servers_accept_sockets_array[i], full_request, size_of_full_request, 0);

        //recv(servers_accept_sockets_array[i], returned_string, MAX_LEN_RECV, 0);
        //returned_string = read_server_message(temp);
        //send(accept_client_socket, returned_string, MAX_LEN_RECV, 0);

        i = (i==3) ? 0 : (i + 1);

        break;
    }



}
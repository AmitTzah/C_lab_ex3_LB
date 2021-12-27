#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>


#define MAX_LEN_RECV 1024 // I'm not sure what should we use
#define MIN_PORT_VAL 1024
#define MAX_PORT_VAL 64000
#define LISTEN_BACKLOG 10
#define NUMBER_OF_SERVERS 3
#define LOCAL_HOST "127.0.0.1"

#define handle_error(msg) \
           do { perror(msg); exit(EXIT_FAILURE); } while (0)

const char *request_end = "\r\n\r\n";

int ranged_rand(int lower, int upper)
{
    return (rand() % (upper - lower + 1)) + lower;
}

char* read_server_message(int server_socket){
    int counter = 0, i = 0;
    char *returned_string, *temp_returned;

    recv(server_socket, temp_returned, MAX_LEN_RECV, 0);
    strcat(returned_string,temp_returned);

    while(counter < 2){
        if (strcmp(temp_returned + i, "\r\n\r\n") == 0)
            counter++;
        if (temp_returned[i] == '\0') {
            recv(server_socket, temp_returned, MAX_LEN_RECV, 0);
            strcat(returned_string,temp_returned);
        }
        i++;
    }

    return returned_string;
}

int check_if_request_completed(char* request) {
    int i = 0;
    while(!((request[i] == '\r') && (request[i+1] =='\n') && (request[i+2] == '\r') && (request[i+3] =='\n'))) {
        i++;
        if (i == strlen(request)-1)
            return 0;
    }
    return 1;
}

int main() {

    int port_server, port_client, servers_accept_sockets_array[3], accept_client_socket; // the sockets returned from accept
    int addr_len = sizeof(struct sockaddr_in);
    char buff[MAX_LEN_RECV], *full_request, *returned_string;
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

    // Accepting http connection
    accept_client_socket = accept(main_client_socket, (struct sockaddr *) &service_client, &addr_len);
    while(accept_client_socket < 0) { // failed to accept, try again
        listen(main_client_socket, LISTEN_BACKLOG);
        accept_client_socket = accept(main_client_socket, (struct sockaddr *) &service_client, &addr_len);
    }

    i = 0;
    while (1){

        do {
            recv(accept_client_socket, buff, MAX_LEN_RECV, 0); // recives string from http untill \r\n\r\n
            full_request = strcat(full_request, buff);
            printf("%s\n",buff);
        }while (check_if_request_completed(full_request) == 0);
        printf("%s\n",full_request);

        send(servers_accept_sockets_array[i], full_request, MAX_LEN_RECV, 0); //sends the string to next server by order
        recv(servers_accept_sockets_array[i], returned_string, MAX_LEN_RECV, 0);
        //returned_string = read_server_message(temp);
        send(accept_client_socket, returned_string, MAX_LEN_RECV, 0);
        i = (i==3) ? 0 : i + 1;
        break;
        /* not sure if break is needed */
    }



}
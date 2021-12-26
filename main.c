#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#define MAX_LEN_RECV 1024 // I'm not sure what should we use
#define MIN_PORT_VAL 1024
#define MAX_PORT_VAL 64000
#define BACKLOG 10

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
    int temp;
    int port_server, port_client, servers_array[3], client_connected; // the sockets returned from accept
    int addr_len = sizeof(struct sockaddr_in);
    char buff[MAX_LEN_RECV], *full_request, *returned_string;
    FILE *server_port_number, *client_port_number;
    server_port_number = fopen("server_port.txt", "w");
    client_port_number = fopen("http_port.txt", "w");

    //create a new socket for servers and for client
    int sock_client = socket(AF_INET, SOCK_STREAM, 0);
    int sock_server = socket(AF_INET, SOCK_STREAM, 0);

    //create sockaddr variable with random porn number
    port_server = ranged_rand(MIN_PORT_VAL, MAX_PORT_VAL);
    port_client = ranged_rand(MIN_PORT_VAL, MAX_PORT_VAL);

    struct sockaddr_in service_client, service_server;
    service_client.sin_family = AF_INET;
    service_client.sin_addr.s_addr = inet_addr( "127.0.0.1" );
    service_client.sin_port = htons(port_client);

    service_server.sin_family = AF_INET;
    service_server.sin_addr.s_addr = inet_addr( "127.0.0.1" );
    service_server.sin_port = htons(port_server);

    //bind sever socket, keep trying until success
    while (bind(sock_server, (struct sockaddr*)&service_server, sizeof(service_server)) == -1) {
        port_server = ranged_rand(MIN_PORT_VAL, MAX_PORT_VAL);
        service_server.sin_port = htons(port_server);
    }
    fprintf(server_port_number, "%d", port_server);
    fclose(server_port_number);
    if (listen(sock_server, BACKLOG) == -1)
        exit(1);


    //bind client socket, keep trying until success
    while (bind(sock_client, (struct sockaddr*)&service_client, sizeof(service_client)) == -1) {
        port_client = ranged_rand(MIN_PORT_VAL, MAX_PORT_VAL);
        service_client.sin_port = htons(port_client);
    }
    fprintf(client_port_number, "%d", port_client);
    fclose(client_port_number);
    if (listen(sock_client, BACKLOG) == -1)
        exit(1);

    // Waiting till 3 servers are connected
    int i = 0;
    while (i<3){
        servers_array[i] = accept(sock_server, (struct sockaddr*)&service_server, &addr_len);
        if(servers_array[i] > 0 ) {
            i++;
            listen(sock_server, BACKLOG);
        }
        else{
            printf("connection failed, exit");
            exit(1);
        }
    }

    // Accepting http connection
    client_connected = accept(sock_client, (struct sockaddr *) &service_client, &addr_len);
    while(client_connected <= 0) { // failed to accept, try again
        listen(sock_client, BACKLOG);
        client_connected = accept(sock_client, (struct sockaddr *) &service_client, &addr_len);
    }

    i = 0;
    while (1){

        do {
            recv(client_connected, buff, MAX_LEN_RECV, 0); // recives string from http untill \r\n\r\n
            full_request = strcat(full_request, buff);
        }while (check_if_request_completed(full_request) == 0);

        send(servers_array[i], full_request, MAX_LEN_RECV, 0); //sends the string to next server by order
        recv(servers_array[i], returned_string, MAX_LEN_RECV, 0);
        //returned_string = read_server_message(temp);
        send(client_connected, returned_string, MAX_LEN_RECV, 0);
        i = (i==3) ? 0 : i + 1;
        break;
        /* not sure if break is needed */
    }



}
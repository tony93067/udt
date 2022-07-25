#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#define SERVER_PORT 8888
#define BUFFER_SIZE 1500

int main(int argc, char* argv[])
{
    int server_fd, ret;
    struct sockaddr_in server_addr, client_addr;
    int client_length = sizeof(client_addr);

    int recv_size, send_size;
    char buffer[BUFFER_SIZE];

    server_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP); 
    if(server_fd < 0)
    {
        printf("create socket fail!\n");
        return -1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY); 
    server_addr.sin_port = htons(SERVER_PORT);

    ret = bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if(ret < 0)
    {
        printf("socket bind fail!\n");
        return -1;
    }
    // receive client message to get client ip and port number
    memset(buffer, '\0', BUFFER_SIZE);
    if ((recv_size = recvfrom(server_fd, buffer, BUFFER_SIZE, 0, (struct sockaddr*)&client_addr, &client_length)) < 0){
        printf("Couldn't receive\n");
        return -1;
    }
    printf("Received message from IP: %s and port: %i\n",
           inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
    
    printf("Msg from client: %s\n", buffer);
    
    // start to send to client
    printf("Server Start Sending :\n");
    while(1)
    {
        memset(buffer, '\0', BUFFER_SIZE);
        strcpy(buffer, "udp test");
        
        if ((send_size = sendto(server_fd, buffer, strlen(buffer), 0, (struct sockaddr*)&client_addr, client_length)) < 0){
            printf("Can't send\n");
            return -1;
        }
    }
    close(server_fd);
    return 0;
}
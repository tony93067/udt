#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#define SERVER_PORT 8888
#define BUFFER_SIZE 1500

void udp_send(int fd)
{
    char buffer[BUFFER_SIZE];  
    socklen_t len;
    int send_size, recv_size;
    struct sockaddr_in clent_addr;
    memset(buffer, 0, BUFFER_SIZE);
    if((recv_size = recvfrom(fd, buffer, BUFFER_SIZE, 0, (struct sockaddr*)&clent_addr, &len)) == -1)
    {
        printf("recvfrom error\n");
        exit(1);
    }
    printf("%s\n", buffer);
    /*
    while(1)
    {
        memset(buf, 0, BUFFER_SIZE);
        len = sizeof(clent_addr);
        if((send_size = sendto(fd, buf, BUFFER_SIZE, 0, (struct sockaddr*)&clent_addr, &len)) == -1)
        {
            printf("sendto error\n");
            exit(1);
        }
        printf("client:%s\n",buf);  
        printf("server:%s\n",buf); 
    }*/
}


/*
    server:
            socket-->bind-->recvfrom-->sendto-->close
*/

int main(int argc, char* argv[])
{
    int server_fd, ret;
    struct sockaddr_in ser_addr;

    server_fd = socket(AF_INET, SOCK_DGRAM, 0); //AF_INET:IPV4;SOCK_DGRAM:UDP
    if(server_fd < 0)
    {
        printf("create socket fail!\n");
        return -1;
    }

    memset(&ser_addr, 0, sizeof(ser_addr));
    ser_addr.sin_family = AF_INET;
    ser_addr.sin_addr.s_addr = htonl(INADDR_ANY); 
    ser_addr.sin_port = htons(SERVER_PORT);

    ret = bind(server_fd, (struct sockaddr*)&ser_addr, sizeof(ser_addr));
    if(ret < 0)
    {
        printf("socket bind fail!\n");
        return -1;
    }

    udp_send(server_fd);

    close(server_fd);
    return 0;
}
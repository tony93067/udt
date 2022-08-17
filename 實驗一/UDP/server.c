#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/times.h>
#define SERVER_PORT 8888

int main(int argc, char* argv[])
{
    int server_fd, ret, fd;
    struct sockaddr_in server_addr, client_addr;
    struct stat sb;
    socklen_t client_length = sizeof(client_addr);
    // packet size
    int buffer_size = atoi(argv[1]);
    double execute_time;
    char* file_addr;

    clock_t old, new;
    struct tms time_start, time_end;
    if(argc != 2)
    {
        printf("usage : ./server <packet_size>\n");
        exit(1);
    }
    printf("Setting Parameter:\n");
    printf("Packet Size %d\n", buffer_size);
    int recv_size, send_size;
    char buffer[buffer_size];

    server_fd = socket(AF_INET, SOCK_DGRAM, 0); 
    if(server_fd < 0)
    {
        printf("create socket fail!\n");
        return -1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY); 
    server_addr.sin_port = htons(SERVER_PORT);

    if(bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0)
    {
        printf("socket bind fail!\n");
        return -1;
    }
    // receive client message to get client ip and port number
    memset(buffer, '\0', buffer_size);
    if ((recv_size = recvfrom(server_fd, buffer, buffer_size, 0, (struct sockaddr*)&client_addr, &client_length)) < 0){
        printf("Couldn't receive Client data\n");
        return -1;
    }
    printf("recv_size %d\n", recv_size);
    printf("Received message from IP: %s and port: %i\n",
           inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
    
    printf("Msg from client: %s\n", buffer);
    
    // start to send to client
    printf("Server Start Sending :\n");

    // open file
    fd = open("/home/tony/論文code/論文code/file.txt", O_RDONLY, S_IRWXU);
    
    // get file info
    if(fstat(fd, &sb) == -1)
    {
        printf("fstat error\n");
        exit(1);
    }
    // map file to memory
    file_addr = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if(file_addr == MAP_FAILED)
    {
        printf("mmap error\n");
        exit(1);
    }
    int already_sent = 0;
    int start = 0;
    memset(buffer, '\0', buffer_size);
    strcpy(buffer, "Server Reply");
    send_size = sendto(server_fd, buffer, strlen(buffer), 0, (struct sockaddr*)&client_addr, client_length);
    while(already_sent < sb.st_size)
    {
        memset(buffer, '\0', buffer_size);
        if(start == 0)
        {
            if((old = times(&time_start)) == -1)
            {
                printf("time error\n");
                exit(1);
            }
        }
        if ((send_size = sendto(server_fd, file_addr + already_sent, buffer_size, 0, (struct sockaddr*)&client_addr, client_length)) < 0){
            printf("Can't send\n");
            return -1;
        }
        //printf("send size :%d\n", send_size);
        already_sent += send_size;
        if(sb.st_size - already_sent < buffer_size)
        {
            printf("sb.st_size - already_sent %ld %d\n", sb.st_size, already_sent);
            buffer_size = sb.st_size - already_sent;
        }
        start++;
    }
    if((new = times(&time_end)) == -1)
    {
        printf("time error\n");
        exit(1);
    }
    /*if(munmap(file_addr, sb.st_size) == -1)
    {
        printf("munmap error\n");
        exit(1);
    }*/
    execute_time = (double)(new - old)/sysconf(_SC_CLK_TCK);
    printf("Execute Time : %f\n", execute_time);
    printf("Total Sent Size %d\n", already_sent);
    close(server_fd);
    return 0;
}
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/times.h>
#include <sys/mman.h>
#include <arpa/inet.h>


#define DIE(x) perror(x),exit(1)
#define PORT 12000
#define BUFFER_SIZE 10000

char* sys_time;
/***global value***/
int num_client = 1;
int data_size = 0;
struct tms time_start,time_end;
clock_t old_time, new_time;
double ticks;
int Background_TCP_Number = 0;
//int cd = 0;
/******************/

void *send_packet(void *arg)
{
    int fd, ex; // file descriptor
    char buffer[BUFFER_SIZE];
    struct stat sb;
    int j = 0;
    // send function return value
    long long int send_size = 0;
    long long int recv_size = 0;
    // client connection file descriptor
    int cd = *((int *)arg);
    // record total send data size
    long long int total_recvsize = 0;
    num_client++;
    printf("Start Recving Packet!\n");

    // open file
    fd = open("file.txt", O_CREAT|O_RDWR|O_TRUNC, S_IRWXU);
    ex = open("TCP_Receiver.csv", O_CREAT|O_RDWR|O_APPEND, S_IRWXU);
    
    while(1)
    {
        memset(buffer, '\0', BUFFER_SIZE);
        //recv packet
        if((recv_size = recv(cd, (char *)buffer, BUFFER_SIZE, 0)) < 0)
        {
            DIE("send");
        }
        if(j == 0)
        {
            if((old_time = times(&time_start)) == -1)
            {
                printf("time error\n");
                exit(1);
            }
            j++;
        }
        if(write(fd, buffer, recv_size) < 0)
        {
            printf("write error\n");
            exit(1);
        }
        total_recvsize += recv_size;
        if(recv_size == 0)
        {
            printf("recv finish\n");
            close(fd);
            break;
        }
    }
    if((new_time = times(&time_end)) == -1)
    {
        printf("time error\n");
        exit(1);
    }
    ticks = sysconf(_SC_CLK_TCK);
    double execute_time = (new_time - old_time)/ticks;
    printf("Execute Time: %2.2f\n", execute_time);
    printf("total recvsize %lld \n", total_recvsize);
    /*************/
    char str[100] = {0};
    sprintf(str, "%s\n", "TCP");
    write(ex, str, sizeof(str));
    
    memset(str, '\0', sizeof(str));
    sprintf(str, "%s\t%s\n", "程式執行時間", sys_time);
    write(ex, sys_time, strlen(sys_time));
    
    memset(str, '\0', sizeof(str));
    sprintf(str, "%s\t%d\n", "Background_TCP_Number", Background_TCP_Number);
    write(ex, str, sizeof(str));
    
    memset(str, '\0', sizeof(str));
    sprintf(str, "%s\t%f\n", "Recv Time", execute_time);
    write(ex, str, sizeof(str));
    
    write(ex, "\n", sizeof("\n"));
    printf("Packet recv sucessfully!\n\n");	
    //fprintf(ex, "%s\n", "TCP");
    //fprintf(ex, "%s\t%f\n", "Recv Time", execute_time);
    //close connection
    close(ex);
    close(cd);
    pthread_exit(0);
}

int main(int argc, char **argv)
{
    time_t now = time(0);
    sys_time = ctime(&now);
    static struct sockaddr_in server;
    int sd,cd; 
    int reuseaddr = 1;
//   char buffer[BUFFER_SIZE];
//   int send_size = 0;
    int client_len = sizeof(struct sockaddr_in);
//   int i = 0,j = 0;
    pthread_t p1;
    int ret = 0;
    Background_TCP_Number = atoi(argv[1]);

    if(argc != 2)
    {
        printf("Usage: %s %s\n",argv[0], "Background_TCP_Number");
        exit(1);
    }

    //open socket
    sd = socket(AF_INET,SOCK_STREAM, 0);
    if(sd < 0)
    {
        DIE("socket");
    }

    /* Initialize address. */
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    server.sin_addr.s_addr = inet_addr("140.117.171.182");

    //reuse address
    setsockopt(sd,SOL_SOCKET,SO_REUSEADDR,&reuseaddr,sizeof(reuseaddr));

    //bind
    if(bind(sd,(struct sockaddr *)&server,sizeof(server)) < 0)
    {
        DIE("bind");
    }

    //listen
    if(listen(sd,1) < 0)
    {
        DIE("listen");
    }
   
    //num_packets = atoi(argv[1]);
    while(1)
    {
        cd = accept(sd,(struct sockaddr *)&server,&client_len);
	printf("accept");
        if((ret = pthread_create(&p1, NULL, send_packet, (void*)&cd)) != 0)
        {
            fprintf(stderr, "can't create p1 thread:%s\n", strerror(ret));
            exit(1);
        }
    }
    //close connection
    close(sd);
  
  return 0;
}


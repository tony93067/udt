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


#define DIE(x) perror(x),exit(1)
#define PORT 8888


/***global value***/
int num_packets = 0;
int num_client = 1;
int data_size = 0;
int data_interval = 0;
struct tms time_start,time_end;
clock_t old_time, new_time;
double ticks;
//int cd = 0;
/******************/

void *send_packet(void *arg)
{
    int fd; // file descriptor
    struct stat sb;
    int j = 0;
    // send function return value
    long long int send_size = 0;
    // client connection file descriptor
    int cd = *((int *)arg);
    // record total send data size
    long long int total_sendsize = 0;
    // used to get mmap return virtual address
    char* file_addr;
    num_client++;
    printf("Start Sending Packet!\n");

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

    if(j == 0)
    {
        if((old_time = times(&time_start)) == -1)
        {
            printf("time error\n");
            exit(1);
            }
    }
    //send packet
    if((send_size = send(cd, (char *)file_addr, sb.st_size, 0)) < 0)
    {
        DIE("send");
    }
    total_sendsize += send_size;
    if(munmap(file_addr, sb.st_size) == -1)
    {
        printf("munmap error\n");
        exit(1);
    }
    close(fd);
    if((new_time = times(&time_end)) == -1)
    {
        printf("time error\n");
        exit(1);
    }
    ticks = sysconf(_SC_CLK_TCK);
    double execute_time = (new_time - old_time)/ticks;
    printf("Execute Time: %2.2f\n", execute_time);
    printf("total sendsize %lld \n", total_sendsize);
    /*************/
    printf("Packet send sucessfully!\n\n");	

    //close connection
    close(cd);
    pthread_exit(0);
}

int main(int argc, char **argv)
{
    static struct sockaddr_in server;
    int sd,cd; 
    int reuseaddr = 1;
//   char buffer[BUFFER_SIZE];
//   int send_size = 0;
    int client_len = sizeof(struct sockaddr_in);
//   int i = 0,j = 0;
//   int send_packet = 0;
    pthread_t p1;
    int ret = 0;
//   pthread_mutex_t work = PTHREAD_MUTEX_INITIALIZER;

    if(argc != 1)
    {
        printf("Usage: %s\n",argv[0]);
        exit(1);
    }

    //open socket
    sd = socket(AF_INET,SOCK_STREAM,0);
    if(sd < 0)
    {
        DIE("socket");
    }

    /* Initialize address. */
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    server.sin_addr.s_addr = htonl(INADDR_ANY);

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


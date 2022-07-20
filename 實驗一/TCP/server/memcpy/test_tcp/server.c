#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/times.h>



#define DIE(x) perror(x),exit(1)
#define PORT 1290
#define BUFFER_SIZE 1500


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
    int fd;
    int i = 0;
    int j = 0;
    char buffer[BUFFER_SIZE];
    int send_size = 0;
    int send_packet = 0;
    int cd = *((int *)arg);
    int no_client = num_client;
    int total_sendsize = 0;
    int return_value = 0;
    //int cd = (int *)&arg;
    num_client++;
    printf("Client %d: Start Sending Packet!\n",no_client);
    printf("Data Size: %d\n", data_size);
    /*make packet & send packet*/
    i = 0;
    // open file
    fd = open("/home/tony/論文code/論文code/file.txt", O_RDONLY, S_IRWXU);
    while(1)
    {
        memset(buffer, '\0', BUFFER_SIZE);
        if(j == 0)
   		{
   		    if((old_time = times(&time_start)) == -1)
   		    {
        		printf("time error\n");
        		exit(1);
      			}
   		}
        if((return_value = read(fd, buffer, data_size)) == -1)
        {
            printf("Read error\n");
            exit(1);
        }
        //printf("%s enter\n", buffer);
        if(return_value == 0)
            break;
        //send packet
        if((send_size = send(cd, (char *)buffer, return_value, 0)) < 0)
        {
            DIE("send");
        }
        //printf("send size %d\n", send_size);
        send_packet++;
        total_sendsize += send_size;
        // sleep 0.025s(use to control sending rate)
        usleep(data_interval);
        j++;
    }
    close(fd);
    if((new_time = times(&time_end)) == -1)
    {
        printf("time error\n");
        exit(1);
    }
    ticks=sysconf(_SC_CLK_TCK);
    double execute_time = (new_time - old_time)/ticks;
    printf("Execute Time: %2.2f\n", execute_time);
    printf("total sendsize %d \n", total_sendsize);
    /*************/
    printf("Client %d: Total Send Packet: %d\n",no_client,send_packet);
    printf("Client %d: Packet send sucessfully!\n\n",no_client);	

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

    if(argc != 3)
    {
        printf("Usage: %s <packet_size> <data_interval(us)>\n",argv[0]);
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
    data_size = atoi(argv[1]);
    data_interval = atoi(argv[2]);

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


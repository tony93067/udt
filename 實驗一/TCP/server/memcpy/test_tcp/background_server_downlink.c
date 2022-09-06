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

#define BUFFER_SIZE 10000
#define DIE(x) perror(x),exit(1)
#define PORT 9999


/***global value***/
int num_client = 1;
//int cd = 0;
/******************/

void *send_packet(void *arg)
{
    int j = 0;
    // send function return value
    int cd = *((int *)arg);
    num_client++;
    printf("Start Receiving Packet!\n");
    char* buffer = (char*)malloc(sizeof(char)*BUFFER_SIZE);
    //send packet
    while(1)
    {
        memset(buffer, '1', BUFFER_SIZE);
        if(send(cd, buffer, BUFFER_SIZE, 0) < 0)
        {
            DIE("send");
        }
    }
    //close connection
    close(cd);
    pthread_exit(0);
}

int main(int argc, char **argv)
{
    static struct sockaddr_in server;
    int sd, cd, i;
    int reuseaddr = 1;
    int client_len = sizeof(struct sockaddr_in);
    pthread_t p[100];
    int ret = 0;

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
        if((ret = pthread_create(&p[i], NULL, send_packet, (void*)&cd)) != 0)
        {
            fprintf(stderr, "can't create thread:%s\n", strerror(ret));
            exit(1);
        }else
        	printf("client %d\n", i);
        i++;
    }
    //close connection
    close(sd);
  
  return 0;
}


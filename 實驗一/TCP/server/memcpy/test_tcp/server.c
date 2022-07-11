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


#define DIE(x) perror(x),exit(1)
#define PORT 1290
#define BUFFER_SIZE 1500

/***global value***/
int num_packets = 0;
int num_client = 1;
//int cd = 0;
/******************/

void *send_packet(void *arg)
{
  int i = 0;
  int j = 0;
  char buffer[BUFFER_SIZE];
  int send_size = 0;
  int send_packet = 0;
  int cd = *((int *)arg);
  int no_client = num_client;
  //int cd = (int *)&arg;
  num_client++;
  printf("Client %d: Start Sending Packet!\n",no_client);
  printf("BUFFER_SIZE: %ld\n", sizeof(buffer));
   /*make packet & send packet*/
   i = 0;
   for (j = 0; j < num_packets ; j++)//number of packet
   {
      //fill data in packet
      if(i > 255)//data: 0-255
        i = 0;

      memset(buffer,i,sizeof(buffer));//fill data in buffer
      i++;//next data

      //send packet
      if((send_size = send(cd,(char *)buffer,sizeof(buffer),0)) < 0)
      {
        DIE("send");
      }
      send_packet++;
      
      // sleep 0.025s(use to control sending rate)
     usleep(1000);
   }
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

   if(argc != 2)
   {
     printf("Usage: %s <num_packets>\n",argv[0]);
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
   
   num_packets = atoi(argv[1]);

   while(1)
   {
   //accept
/*   if((cd = accept(sd,(struct sockaddr *)&server,&client_len)) == -1)
   {
     DIE("accept");
   }
*/
//    pthread_mutex_lock(&work); 
     cd = accept(sd,(struct sockaddr *)&server,&client_len);
//    pthread_mutex_unlock(&work);
//     run_times = atoi(argv[1]);

//   printf("Start Sending!\nPacket Data Size: %d\n",(int)sizeof(buffer));
   /*create thread to handle each client, one receiver use one different protocol*/
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


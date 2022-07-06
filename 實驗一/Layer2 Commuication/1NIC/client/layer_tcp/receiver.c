#include <netinet/if_ether.h>
#include <net/ethernet.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <linux/netlink.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/times.h>
#include <pthread.h>
#include <netpacket/packet.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <errno.h>


#define DIE(x) perror(x),exit(1) 
#define DEVICE_NAME "eth0"
#define MAX_PAYLOAD 1440 /*maximum payload size*/

/*protocol*/
#define ETH_P_PROT1 0x1111
#define ETH_P_PROT2 0x3333

#define ETH_HEADER 14
#define PORT 1235

/*function prototype*/
void get_mac(int sock,char *interface,unsigned char *mac);

/****
**	Global var
*****/

struct protocol_packet
{
  struct ether_header eth_hdr;
  int protocol;
};

struct packet
{
  struct ether_header eth_hdr;
  char packet_data[MAX_PAYLOAD];
};

struct protocol_packet prot_pack;//packet use for exchange protocol of client
/****/

int main(int argc, char* argv[])
{	
  int tcpsd,sockfd2;
  static struct sockaddr_in server;
  struct hostent *host;
  char server_name[512];
  unsigned char sender_mac[6];
  struct sockaddr_ll send_addr;
  struct packet recv_pack;
  int i = 0;
  int recv_size = 0;
  clock_t old,new;//use for count executing time
  struct tms time_start,time_end;//use for count executing time
  double ticks;
  int recv_packet = 0;
  int send_size = 0;
  struct ifreq s_ifr;
  int i_ifindex;
  int ec;
 
  if(argc != 4)
  {
    printf("Usage: %s <sender_hw_addr> <sender_ip> <run_times>\n",argv[0]);
    exit(1);
  }

  //open tcp socket(use for exchange protocol)
  if((tcpsd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {	
    DIE("socket");
  }
  strcpy(server_name,argv[2]);//set server
  /* Set up destination address. */
  server.sin_family = AF_INET;
  host = gethostbyname(server_name);
  server.sin_port = htons(PORT);    
  memcpy((char*)&server.sin_addr,host->h_addr_list[0],host->h_length);

  /*set MAC addr*/
  get_mac(tcpsd,DEVICE_NAME,sender_mac);
  set_sender_hardware_addr(&prot_pack.eth_hdr,sender_mac);  		 
  set_target_hardware_addr(&prot_pack.eth_hdr,argv[1]);	

  //set protocol(data)
  prot_pack.protocol = ETH_P_PROT2;

  /* Connect to the server. */
  if(connect(tcpsd,(struct sockaddr*)&server,sizeof(server)) == -1)
  {
    DIE("connect");
  }

  /*send protocol*/
  if((send_size = send(tcpsd,(struct protocol_packet *)&prot_pack,sizeof(prot_pack),0)) < 0)
  {
      DIE("send");
  }
  
  //open socket2(use for exchange data)
  if((sockfd2 = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_PROT2))) < 0)//Cindy
  {	
    DIE("socket");
  }


  printf("\nStart Receiving!\nPacket Data Size: %d\n",(int)sizeof(recv_pack.packet_data));

  /*receive packet*/
  int error = 0;
  int j = 0;
  int run_times = atoi(argv[3]);

  for(j = 0;j<run_times;j++)
  {
    if((recv_size = recvfrom(sockfd2, &recv_pack, sizeof(recv_pack), 0, NULL, NULL)) <= 0)
    {
      DIE("recvfrom");
    }
    else
    {
      recv_packet++;
    }

    if(j == 0)//wait until receiving first packet and keep the first time
    {

      //start time
      if((old = times(&time_start)) == -1)
      {
        DIE("times error\n");
      }
    }
  
    if((recv_size - ETH_HEADER) != MAX_PAYLOAD)//if data size is not equal to MAX_PAYLOAD, means data loss, so plus error
    {
      error++;
    }
  }
  //finish time
  if((new = times(&time_end)) == -1)
  {
    DIE("times error\n");
  }

  /************************/
  
  printf("\n[Result]:\n");
  //executing time
  ticks=sysconf(_SC_CLK_TCK);
  printf("Run Time: %2.2f\n",(double)(new-old)/ticks);

  printf("Recv Packet: %d\n",recv_packet);
  printf("Error: %d\n",error);
  if(recv_packet == run_times)
  {
    printf("No data loss!\n\n");
  }
  else
  {
    printf("Data loss!\n\n");
  }

  //close connection
  close(tcpsd);
  close(sockfd2);
  return 0;
}

void get_mac(int sock,char *interface,unsigned char *mac)
{
  struct ifreq ifreq1;

  strcpy(ifreq1.ifr_name,interface);//set interface's name
  
  if(ioctl(sock,SIOCGIFHWADDR,&ifreq1) < 0)
  {
    printf("Unable to get MAC address\n");
    exit(1);
  }
  memcpy(mac,ifreq1.ifr_hwaddr.sa_data,6);

}


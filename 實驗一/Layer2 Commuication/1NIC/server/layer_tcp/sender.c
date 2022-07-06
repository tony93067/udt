#include <netinet/if_ether.h>
#include <net/ethernet.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <linux/netlink.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <fcntl.h>
#include <netpacket/packet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <netinet/in.h> 
#include <errno.h>

#define DEVICE_NAME "eth0"
#define MAX_PAYLOAD 1440 /*maximum payload size*/
#define DIE(x) perror(x),exit(1)

/*protocol*/
#define ETH_P_PROT1 0x1111

#define PORT 1235

/*function prototype*/
void print_usage();
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

struct protocol_packet prot_pack;//packet use for exchange protocol of receiver
unsigned char sender_mac[6];
int run_times;
int num_client = 1;
/****/


void *handle_protocol(void *arg)
{
  int sfd;
  struct protocol_packet *data;
  int protocol;
  struct sockaddr_ll addr;  
  int ec;
  struct ifreq s_ifr;
  int i_ifindex;
  struct packet send_pack;
  int i = 0;
  int j = 0;
  int send_size = 0;
  int send_packet = 0;
  int client_no = num_client;

  num_client++;

  
  data = (struct protocol_packet *) arg;
  protocol = data->protocol;
  
  /*create new socket for one client*/
  if((sfd = socket(PF_PACKET,SOCK_RAW,protocol)) < 0)
  {
    DIE("socket");
  }
  
  //clear structure
  memset(&addr, 0, sizeof(struct sockaddr_ll));

  /* initialize interface struct */
  strncpy(s_ifr.ifr_name, DEVICE_NAME, sizeof(s_ifr.ifr_name));

  /* get NIC's index*/
  ec = ioctl(sfd, SIOCGIFINDEX, &s_ifr);
  if(ec == -1)
  {
    DIE("ioctl");
  }

  //fill addr
  addr.sll_ifindex = s_ifr.ifr_ifindex;

  /*set MAC addr*/
  get_mac(sfd,DEVICE_NAME,sender_mac);
  set_sender_hardware_addr(&send_pack.eth_hdr,sender_mac);  		 
  set_target_hardware_addr(&send_pack.eth_hdr,data->eth_hdr.ether_shost);	


  //set packet's ether_header frame type "protocol"
  send_pack.eth_hdr.ether_type = protocol;

  #ifdef DEBUG
    printf("Ethernet des. address: ");
    for(i =0; i<6; i++)
      printf("%02x ",send_pack.eth_hdr.ether_dhost[i]);
    printf("\n");

    printf("Ethernet src. address: ");
    for(i=0; i<6; i++)
      printf("%02x ",send_pack.eth_hdr.ether_shost[i]);
    printf("\n");
		
    printf("frame type: %x\n",send_pack.eth_hdr.ether_type);
    printf("packet size: %d\n",(int)sizeof(send_pack));
  #endif

  printf("Client %d: Start Sending Packet.\n",client_no);
   /*make packet & send packet*/
  i = 0;
  for (j = 0; j<run_times ; j++)//number of packet
  {

    /*create packet data*/
    if(i > 255)//packet data(0-255)
      i = 0;
    memset(send_pack.packet_data,i,sizeof(send_pack.packet_data));//fill packet's data
    i++;//next data
    /********************/

    if((send_size = sendto(sfd, &send_pack, sizeof(struct ether_header)+sizeof(send_pack.packet_data), MSG_DONTROUTE, (struct sockaddr*)&addr, sizeof(addr))) <= 0)
    {
      DIE("sendto");
    }
    else
    {
      send_packet++;
    }
  }

  /****************************/
  printf("Send Packet Number: %d\n",send_packet);
  printf("Packet send sucessfully!\n\n");	

  //close connection
  close(sfd);
}


int main(int argc, char* argv[])
{	
  int tcpsd,cd;
  int recv_size = 0;
  static struct sockaddr_in server;
  int reuseaddr = 1;
  int client_len = sizeof(struct sockaddr_in);
  pthread_t p1;
  int ret = 0;

  if(argc != 2)
  {
    print_usage();
    exit(1);
  }

  run_times = atoi(argv[1]);

  //open socket
  if((tcpsd = socket(AF_INET, SOCK_STREAM, 0)) < 0)//tcp to transfer protocol
  {	
    DIE("scoket");
  }
  
  /* Initialize address. */
   server.sin_family = AF_INET;
   server.sin_port = htons(PORT);
   server.sin_addr.s_addr = htonl(INADDR_ANY);

   //reuse address
   setsockopt(tcpsd,SOL_SOCKET,SO_REUSEADDR,&reuseaddr,sizeof(reuseaddr));

   //bind
   if(bind(tcpsd,(struct sockaddr *)&server,sizeof(server)) < 0)
   {
     DIE("bind");
   }

   //listen
   if(listen(tcpsd,15) < 0)
   {
     DIE("listen");
   }
 
   while(1)
   {
      printf("Wait Receiver!\n");
   //accept
   if((cd = accept(tcpsd,(struct sockaddr *)&server,&client_len)) == -1)
   {
     DIE("accept");
   }

   //send packet
   if((recv_size = recv(cd,(struct protocol_packet *)&prot_pack,sizeof(prot_pack),0)) < 0)
   {
     DIE("recv");
   }

   /*create thread to handle each client, one receiver use one different protocol*/
   if((ret = pthread_create(&p1, NULL, handle_protocol, (void*)&prot_pack)) != 0)
   {
     fprintf(stderr, "can't create p1 thread:%s\n", strerror(ret));
     exit(1);
   }
  }

  //close connection
  close(tcpsd);
  return 0;
}

void print_usage()
{
  printf("usage: ./sender <run_times>\n");
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


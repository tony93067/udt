#include "func.h"
#include <netinet/if_ether.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

/* Set sender hardware address */
void set_sender_hardware_addr(struct ether_header *packet,unsigned char *address)
{
  int i = 0;
  
  memcpy(packet->ether_shost,address,6);	
#ifdef DEBUG
  printf("send hardware address : ");
  for(i=0; i<6; i++)
    printf("%02x ",packet->ether_shost[i]);
  printf("\n");
#endif
}


void set_target_hardware_addr(struct ether_header *packet, unsigned char *address)
{
    /*check the hardware address format
    **   XX:XX:XX:XX:XX:XX
    **   str_length = 17
    */
/*    if(strlen(address) != 17)
    {
      printf("invalid target hardware address\n");
      exit(1);
    }
	
    int i=0;
    char *ip_byte;
    char hex[5] = "0x00";
    ip_byte = strtok(address,":");
	 
    for(i=0; i<6; i++)
    {
      if(ip_byte == NULL || strlen(ip_byte) > 2)
      {
        printf("invalid target hardware address\n");
        exit(1);
      }
      //for traslating hex to int, assign "0x" to each ip section 
      sprintf((char*)hex,"0x%s",ip_byte);
      packet->ether_dhost[i] = (unsigned int)strtol((char*)hex,(char**)NULL,0);
      if(strcmp(hex,"0x00") !=0  && packet->ether_dhost[i] == 0)
      {
        printf("invalid target hardware address\n");
        exit(1);
      }

      ip_byte = strtok(NULL," :");		
    }
*/
   int i = 0;
   memcpy(packet->ether_dhost,address,6);
#ifdef DEBUG
    printf("target hardware address : ");
    for(i=0; i<6; i++)
      printf("%02x ",packet->ether_dhost[i]);
    printf("\n");
#endif
}


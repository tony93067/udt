#ifndef __FUNC_H__
#define __FUNC_H__

#include <netinet/if_ether.h>
void set_sender_hardware_addr(struct ether_header *packet, unsigned char *address);
void set_target_hardware_addr(struct ether_header *packet, char *address);
#endif

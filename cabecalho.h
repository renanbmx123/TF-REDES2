#include<sys/socket.h>
#include<sys/types.h>
#include<sys/ioctl.h>

#include<net/if.h>
#include<netinet/in.h>
#include<netinet/ip.h>
#include<netinet/if_ether.h>
#include<netinet/udp.h>

#include<linux/if_packet.h>

#include<arpa/inet.h>

struct cabecalho
  {
	uint16_t numseq;    
	uint16_t tam;
	uint16_t flags;
  };

struct recv{
	struct ethhdr *eth;
	struct iphdr *iph;
	struct udphdr *udph;
	struct cabecalho cab;
};

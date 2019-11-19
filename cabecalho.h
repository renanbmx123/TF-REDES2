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
	uint16_t numseq;	//numero de sequencia do pacote
	uint16_t tam;			//tamanho dos dados armazenados
	uint16_t flags;		//flags de comunicação
	uint16_t checksum;//checksum
  };

struct recv{
	struct ethhdr *eth;
	struct iphdr *iph;
	struct udphdr *udph;
	struct cabecalho cab;
};

/*-------------------------------------------------------------*/
/* Exemplo Socket Raw - Captura pacotes recebidos na interface */
/*-------------------------------------------------------------*/

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <string.h>
#include <unistd.h>

/* Diretorios: net, netinet, linux contem os includes que descrevem */
/* as estruturas de dados do header dos protocolos   	  	        */

#include <net/if.h>  //estrutura ifr
#include <netinet/ether.h> //header ethernet
#include <netinet/in.h> //definicao de protocolos
#include <arpa/inet.h> //funcoes para manipulacao de enderecos IP

#include <netinet/in_systm.h> //tipos de dados

#include "cabecalho.h"

#define BUFFSIZE 1518

// Atencao!! Confira no /usr/include do seu sisop o nome correto
// das estruturas de dados dos protocolos.

  unsigned char buff1[BUFFSIZE],buff2[BUFFSIZE]; // buffer de recepcao

  int sockd;
  int on;
  struct ifreq ifr,ifreq_c, ifreq_ip;
	struct iphdr *iph;
	struct udphdr *udph;
	struct cabecalho cab;
	//struct recv recev;
	int total_len = 0, send_len = 0,tam=0;
	char ifName[100];
#define DESTMAC0	0x08
#define DESTMAC1	0x00
#define DESTMAC2	0x27
#define DESTMAC3	0x56
#define DESTMAC4	0x75
#define DESTMAC5	0x1a

#define destination_ip "10.0.2.15"


/*void get_mac(){
	memset(&ifreq_c,0,sizeof(ifreq_c));
	strncpy(ifreq_c.ifr_name,"eth0",IFNAMSIZ-1);
//if(ioctl(sockd, SIOCGIFINDEX, &ifr) 
	if((ioctl(sockd,SIOCGIFHWADDR,&ifreq_c))<0)
		printf("error in SIOCGIFHWADDR ioctl reading");

	printf("Mac= %.2X-%.2X-%.2X-%.2X-%.2X-%.2X\n",(unsigned char)(ifreq_c.ifr_hwaddr.sa_data[0]),(unsigned char)(ifreq_c.ifr_hwaddr.sa_data[1]),(unsigned char)(ifreq_c.ifr_hwaddr.sa_data[2]),(unsigned char)(ifreq_c.ifr_hwaddr.sa_data[3]),(unsigned char)(ifreq_c.ifr_hwaddr.sa_data[4]),(unsigned char)(ifreq_c.ifr_hwaddr.sa_data[5]));


	printf("ethernet packaging start ... \n");

	struct ethhdr *eth = (struct ethhdr *)(buff1);
  	eth->h_source[0] = (unsigned char)(ifreq_c.ifr_hwaddr.sa_data[0]);
  	eth->h_source[1] = (unsigned char)(ifreq_c.ifr_hwaddr.sa_data[1]);
   	eth->h_source[2] = (unsigned char)(ifreq_c.ifr_hwaddr.sa_data[2]);
   	eth->h_source[3] = (unsigned char)(ifreq_c.ifr_hwaddr.sa_data[3]);
   	eth->h_source[4] = (unsigned char)(ifreq_c.ifr_hwaddr.sa_data[4]);
   	eth->h_source[5] = (unsigned char)(ifreq_c.ifr_hwaddr.sa_data[5]);
	//buff1[6],buff1[7],buff1[8],buff1[9],buff1[10],buff1[11]
   	eth->h_dest[0]    =  buff1[6];//DESTMAC0;
   	eth->h_dest[1]    =  buff1[7];//DESTMAC1;
   	eth->h_dest[2]    =  buff1[8];//DESTMAC2;
  	eth->h_dest[3]    =  buff1[9];//DESTMAC3;
   	eth->h_dest[4]    =  buff1[10];//DESTMAC4;
   	eth->h_dest[5]    =  buff1[11];//DESTMAC5;

   	eth->h_proto = htons(ETH_P_IP);   //0x800

   	printf("ethernet packaging done.\n");

		total_len+=sizeof(struct ethhdr);
}

/*void get_udp()
{
  udph->source	= htons(5001);
	udph->dest	= htons(5002);
	udph->check	= 0;

	total_len+= sizeof(struct udphdr);
	get_data();
	udph->len		= htons((total_len - sizeof(struct iphdr) - sizeof(struct ethhdr)));

}*/

/*unsigned short checksum(unsigned short* buff, int _16bitword)
{
	unsigned long sum;
	for(sum=0;_16bitword>0;_16bitword--)
		sum+=htons(*(buff)++);
	do
	{
		sum = ((sum >> 16) + (sum & 0xFFFF));
	}
	while(sum & 0xFFFF0000);

	return (~sum);
}


void get_ip()
{
	memset(&ifreq_ip,0,sizeof(ifreq_ip));
	strncpy(ifreq_ip.ifr_name,ifName,IFNAMSIZ-1);
  	 if(ioctl(sockd,SIOCGIFADDR,&ifreq_ip)<0)
 	 {
		printf("error in SIOCGIFADDR \n");
	 }

	iph->ihl	= 5;
	iph->version	= 4;
	iph->tos	= 16;
	iph->id		= htons(10201);
	iph->ttl	= 64;
	iph->protocol	= 17;
	iph->saddr	= inet_addr(inet_ntoa((((struct sockaddr_in *)&(ifreq_ip.ifr_addr))->sin_addr)));
	iph->daddr	= inet_addr(destination_ip); // put destination IP address
	total_len += sizeof(struct iphdr);
	get_udp();
	iph->tot_len	= htons(total_len - sizeof(struct ethhdr));
  //printf("tamanho: %d\n",htonl(iph->tot_len));
	iph->check = 0;
	iph->check	= htons(checksum((unsigned short*)(buff1 + sizeof(struct ethhdr)), (sizeof(struct iphdr)/2)));

}/**/






int main(int argc,char *argv[])
{
    /* Criacao do socket. Todos os pacotes devem ser construidos a partir do protocolo Ethernet. */
    /* De um "man" para ver os parametros.*/
    /* htons: converte um short (2-byte) integer para standard network byte order. */
		unsigned char *data;
		struct ethhdr *eth;
		struct cabecalho *cbl;
    if((sockd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) < 0) {
       printf("Erro na criacao do socket.\n");
       exit(1);
    }

	// O procedimento abaixo eh utilizado para "setar" a interface em modo promiscuo
	//strcpy(ifr.ifr_name, "eth0");s
	if (argc > 1){
		strcpy(ifr.ifr_name, argv[1]);
		strcpy(ifName,argv[1]);
	}else{
		strcpy(ifr.ifr_name, "eth0");
		strcpy(ifName, "eth0");
	}
	if(ioctl(sockd, SIOCGIFINDEX, &ifr) < 0)
		printf("erro no ioctl!");
	
	ioctl(sockd, SIOCGIFFLAGS, &ifr);
	ifr.ifr_flags |= IFF_PROMISC;
	ioctl(sockd, SIOCSIFFLAGS, &ifr);
	


	// recepcao de pacotes
	while (1) {
			memset(&buff1, 0, sizeof(buff1));
   		tam=recv(sockd,(char *) &buff1, sizeof(buff1), 0x0);
			//recv(sockd,&recev, sizeof(recev), 0x0);
			eth = (struct ethhdr *)(buff1);
			iph = (struct iphdr*)(buff1 + sizeof(struct ethhdr));
  		udph = (struct udphdr *)(buff1 + sizeof(struct iphdr) + sizeof(struct ethhdr));
			cbl=	(struct cabecalho *)(buff1 + sizeof(struct iphdr) + sizeof(struct ethhdr) + sizeof(struct udphdr));
			data=(buff1 + sizeof(struct iphdr) + sizeof(struct ethhdr) + sizeof(struct udphdr)+sizeof(struct cabecalho));
			if(eth->h_proto==htons(ETH_P_IP)){
		// impressão do conteudo - exemplo Endereco Destino e Endereco Origem
			//printf("MAC Destino: %x:%x:%x:%x:%x:%x \n", buff1[0],buff1[1],buff1[2],buff1[3],buff1[4],buff1[5]);
			printf("MAC Destino: %x:%x:%x:%x:%x:%x \n", eth->h_source[0],eth->h_source[1],eth->h_source[2],eth->h_source[3],eth->h_source[4],eth->h_source[5]);
			//printf("MAC Origem:  %x:%x:%x:%x:%x:%x \n\n", buff1[6],buff1[7],buff1[8],buff1[9],buff1[10],buff1[11]);
			printf("MAC Origem:  %x:%x:%x:%x:%x:%x \n\n", eth->h_dest[0],eth->h_dest[1],eth->h_dest[2],eth->h_dest[3],eth->h_dest[4],eth->h_dest[5]);
			printf("data= %s\n",data);
		}
	}
}

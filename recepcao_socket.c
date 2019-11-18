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
#include<errno.h>

/* Diretorios: net, netinet, linux contem os includes que descrevem */
/* as estruturas de dados do header dos protocolos   	  	        */

#include <net/if.h>  //estrutura ifr
#include <netinet/ether.h> //header ethernet
#include <netinet/in.h> //definicao de protocolos
#include <arpa/inet.h> //funcoes para manipulacao de enderecos IP

#include <netinet/in_systm.h> //tipos de dados

#include "cabecalho.h"
//#include "raw.h"
#define BUFFSIZE 1518

// Atencao!! Confira no /usr/include do seu sisop o nome correto
// das estruturas de dados dos protocolos.

  unsigned char buff1[BUFFSIZE],buff2[BUFFSIZE]; // buffer de recepcao

	int sock_raw;
  int sockd,current_ack=0;
  int on;
  struct ifreq ifr,ifreq_c, ifreq_ip,ifreq_i;
	struct iphdr *iph,*iphR;
	struct udphdr *udph,*udphR;
	struct cabecalho cab,cabR;
	struct sockaddr_in source,dest;
	FILE *pFile,*pFile2;
	//struct recv recev;
	int total_len = 0, send_len = 0,tam=0;
	char ifName[100];
	uint8_t numseq=0;
	unsigned char *sendbuff;
	char endFileTransmission = 0;
#define DESTMAC0	0x08
#define DESTMAC1	0x00
#define DESTMAC2	0x27
#define DESTMAC3	0x56
#define DESTMAC4	0x75
#define DESTMAC5	0x1A
#define destination_ip "10.0.2.15"


void get_eth_index()
{
	memset(&ifreq_i,0,sizeof(ifreq_i));
	strncpy(ifreq_i.ifr_name,ifName,IFNAMSIZ-1);

	if((ioctl(sock_raw,SIOCGIFINDEX,&ifreq_i))<0)
		printf("error in index ioctl reading");

	printf("index=%d\n",ifreq_i.ifr_ifindex);

}

void get_mac()
{
	memset(&ifreq_c,0,sizeof(ifreq_c));
	strncpy(ifreq_c.ifr_name,ifName,IFNAMSIZ-1);

	if((ioctl(sock_raw,SIOCGIFHWADDR,&ifreq_c))<0)
		printf("error in SIOCGIFHWADDR ioctl reading");

	printf("Mac= %.2X-%.2X-%.2X-%.2X-%.2X-%.2X\n",(unsigned char)(ifreq_c.ifr_hwaddr.sa_data[0]),(unsigned char)(ifreq_c.ifr_hwaddr.sa_data[1]),(unsigned char)(ifreq_c.ifr_hwaddr.sa_data[2]),(unsigned char)(ifreq_c.ifr_hwaddr.sa_data[3]),(unsigned char)(ifreq_c.ifr_hwaddr.sa_data[4]),(unsigned char)(ifreq_c.ifr_hwaddr.sa_data[5]));


	printf("ethernet packaging start ... \n");

	struct ethhdr *eth = (struct ethhdr *)(sendbuff);
  	eth->h_source[0] = (unsigned char)(ifreq_c.ifr_hwaddr.sa_data[0]);
  	eth->h_source[1] = (unsigned char)(ifreq_c.ifr_hwaddr.sa_data[1]);
   	eth->h_source[2] = (unsigned char)(ifreq_c.ifr_hwaddr.sa_data[2]);
   	eth->h_source[3] = (unsigned char)(ifreq_c.ifr_hwaddr.sa_data[3]);
   	eth->h_source[4] = (unsigned char)(ifreq_c.ifr_hwaddr.sa_data[4]);
   	eth->h_source[5] = (unsigned char)(ifreq_c.ifr_hwaddr.sa_data[5]);

   	eth->h_dest[0]    =  DESTMAC0;
   	eth->h_dest[1]    =  DESTMAC1;
   	eth->h_dest[2]    =  DESTMAC2;
  	eth->h_dest[3]    =  DESTMAC3;
   	eth->h_dest[4]    =  DESTMAC4;
   	eth->h_dest[5]    =  DESTMAC5;

   	eth->h_proto = htons(ETH_P_IP);   //0x800

   	printf("ethernet packaging done.\n");

	total_len+=sizeof(struct ethhdr);


}

void get_data()
{
  int c,aux;
	char arq[512];
	/*|Numseq|TAM 2 Bytes|FLAGS|*/
	printf("\tnumseq=%d\n",current_ack);
	cab.numseq=htons(current_ack);
	printf("\tnumseq=%X\n",ntohs(cab.numseq));
	printf("\tnumseq=%X\n",cab.numseq);
	cab.flags=htons(0x0002);
	printf("\tflags=%X\n",cab.flags);
	printf("len = %d\n",total_len);
	c = fread (arq, sizeof(char), 512, pFile2);
	//total_len=c;
	//printf("Arq= %s\n",arq);
	printf("lido %d bytes\n",c);
	if(c<512){
		printf("\taqui9\n");
		endFileTransmission = 1;
		//cab.flags=htons(0x0001);
	}/**/
	cab.tam=htons(c);
	printf("tam %d \n",ntohs(cab.tam));
	printf("tam %X \n",cab.tam);
	memcpy(sendbuff+total_len, &cab,sizeof(cab));
	total_len+=sizeof(cab);
	memcpy(sendbuff+total_len, arq,c);
	total_len+=c;
	printf("\taqui8\n");
	//printf("%s \n",sendbuff);

}

void get_udp()
{
  udph->source	= htons(5002);
	udph->dest	= htons(5001);
	udph->check	= 0;

	total_len+= sizeof(struct udphdr);
	get_data();
	udph->len		= htons((total_len - sizeof(struct iphdr) - sizeof(struct ethhdr)));

}

unsigned short checksum(unsigned short* buff, int _16bitword)
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
  if(ioctl(sock_raw,SIOCGIFADDR,&ifreq_ip)<0)
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
  printf("tamanho: %d\n",iph->tot_len);
	iph->check = 0;
	iph->check	= htons(checksum((unsigned short*)(sendbuff + sizeof(struct ethhdr)), (sizeof(struct iphdr)/2)));

}
envia(){
		pFile2=fopen ("README.md","r");


	
	sock_raw=socket(AF_PACKET,SOCK_RAW,IPPROTO_RAW);
	if(sock_raw == -1)
		printf("error in socket");
	sendbuff=(unsigned char*)malloc(64); // increase in case of large data.Here data is --> AA  BB  CC  DD  EE
  memset(sendbuff,0,64);
 	unsigned char *aux = sendbuff;

  	//inicializando estruturas.
  iph = (struct iphdr*)(sendbuff + sizeof(struct ethhdr));
  udph = (struct udphdr *)(sendbuff + sizeof(struct iphdr) + sizeof(struct ethhdr));



  get_eth_index();  // interface number
	get_mac();
	// get_ip();

	struct sockaddr_ll sadr_ll;
	sadr_ll.sll_ifindex = ifreq_i.ifr_ifindex;
	sadr_ll.sll_halen   = ETH_ALEN;
	sadr_ll.sll_addr[0]  = DESTMAC0;
	sadr_ll.sll_addr[1]  = DESTMAC1;
	sadr_ll.sll_addr[2]  = DESTMAC2;
	sadr_ll.sll_addr[3]  = DESTMAC3;
	sadr_ll.sll_addr[4]  = DESTMAC4;
	sadr_ll.sll_addr[5]  = DESTMAC5;
	//while(!endFileTransmission){
    printf("sending...\n");

    get_ip();
		printf("ack enviado");
    send_len = sendto(sock_raw,sendbuff, total_len,0,(const struct sockaddr*)&sadr_ll,sizeof(struct sockaddr_ll));

	    if(send_len<0){
			     printf("error in sending....sendlen=%d....errno=%d\n",send_len,errno);
			        return -1;
	           }
    //
    sendbuff = aux;
    total_len = 0;
    send_len = 0;
    total_len+=sizeof(struct ethhdr);
	//}
    fclose (pFile2);
}

int main(int argc,char *argv[])
{
    /* Criacao do socket. Todos os pacotes devem ser construidos a partir do protocolo Ethernet. */
    /* De um "man" para ver os parametros.*/
    /* htons: converte um short (2-byte) integer para standard network byte order. */
		unsigned char *data;
		int stopReceive;
		struct ethhdr *eth,*ethR;
		struct cabecalho *cbl;
		unsigned char *aux;



    if((sockd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) < 0) {
       printf("Erro na criacao do socket.\n");
       exit(1);
    }
		sendbuff=(unsigned char*)malloc(64); // increase in case of large data.Here data is --> AA  BB  CC  DD  EE
			memset(sendbuff,0,64);
		 	aux = sendbuff;
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
	
	/*ioctl(sockd, SIOCGIFFLAGS, &ifr);
	ifr.ifr_flags |= IFF_PROMISC;
	ioctl(sockd, SIOCSIFFLAGS, &ifr);/**/
	
 
	pFile=fopen("RECEBIDO.md","w");
	if(!pFile)
	{
		printf("unable to open log.txt\n");
		return -1;

	}
	// recepcao de pacotes
	while (1) {
			memset(&buff1, 0, sizeof(buff1));
   		tam=recv(sockd,(char *) &buff1, sizeof(buff1), 0x0);
			//recv(sockd,&recev, sizeof(recev), 0x0);
			ethR = (struct ethhdr *)(buff1);
			iphR = (struct iphdr*)(buff1 + sizeof(struct ethhdr));
  		udphR = (struct udphdr *)(buff1 + sizeof(struct iphdr) + sizeof(struct ethhdr));
			cbl=	(struct cabecalho *)(buff1 + sizeof(struct iphdr) + sizeof(struct ethhdr) + sizeof(struct udphdr));
			data=(buff1 + sizeof(struct iphdr) + sizeof(struct ethhdr) + sizeof(struct udphdr)+sizeof(struct cabecalho));
			if(ethR->h_proto==htons(ETH_P_IP)){
				//if(iph->daddr[0]== 10 && iph->daddr[1]== 0 && iph->daddr[2]==  2 && iph->daddr[3]==15 ) {
      		if((ntohs(udphR->dest) == 5002)){
					// impressão do conteudo - exemplo Endereco Destino e Endereco Origem
						//printf("MAC Destino: %x:%x:%x:%x:%x:%x \n", buff1[0],buff1[1],buff1[2],buff1[3],buff1[4],buff1[5]);
						printf("MAC Destino: %x:%x:%x:%x:%x:%x \n", ethR->h_source[0],ethR->h_source[1],ethR->h_source[2],ethR->h_source[3],ethR->h_source[4],ethR->h_source[5]);
						//printf("MAC Origem:  %x:%x:%x:%x:%x:%x \n\n", buff1[6],buff1[7],buff1[8],buff1[9],buff1[10],buff1[11]);
						printf("MAC Origem:  %x:%x:%x:%x:%x:%x \n\n", ethR->h_dest[0],ethR->h_dest[1],ethR->h_dest[2],ethR->h_dest[3],ethR->h_dest[4],ethR->h_dest[5]);
						//printf("data= %s\n",data);
						printf("num seq = %d\n",ntohs(cbl->numseq));
						printf("flag = %4X\n",ntohs(cbl->flags));
						printf("tam = %d\n",ntohs(cbl->tam));
						if(ntohs(cbl->flags)==0x0001){
								stopReceive = 1;
								printf("flagfim\n");
						}
						if(current_ack == ntohs(cbl->numseq)){
							current_ack = ntohs((cbl->numseq));
							current_ack ++;
							printf("ack = %d\n",current_ack );
							int i = 0;

							if(fwrite(data, sizeof(char), ntohs(cbl->tam),pFile) < ntohs(cbl->tam))     /* Escreve a variável NUM | o operador sizeof, que retorna o tamanho em bytes da variável ou do tipo de dados. */
								printf("Erro na escrita do arquivo");

							envia();
					///termina
					if(stopReceive==1)break;
						}	
				}
			//}
			
		}
	}
	fclose(pFile);
	printf("DONE!!!!\n");
}

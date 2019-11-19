/* Note: run this program as root user
 * Author:Subodh Saxena
 */
#include<stdio.h>
#include<string.h>
#include<malloc.h>
#include<errno.h>

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

#include <time.h>
#include <math.h>
#include "cabecalho.h"

#define BUFFSIZE 1518
unsigned char buff1[BUFFSIZE],buff2[BUFFSIZE]; // buffer de recepcao

struct ifreq ifreq_c,ifreq_i,ifreq_ip,ifr; /// for each ioctl keep diffrent ifreq structure otherwise error may come in sending(sendto )
struct iphdr *iph,*iphR; //cabecalho ip de envio e recepcao
struct udphdr *udph,*udphR;//cabecalho UDP de envio e recpcao
struct cabecalho cab; //cabechalho implementado
struct sockaddr_in source,dest;

int sock_raw; 
int sockd;
uint16_t current_ack=1; // valor do ack atual
uint8_t numseq=0; //numero de sequencia do pacote
unsigned char *sendbuff; // buffer de envio
FILE *pFile; // arquivo
char endFileTransmission = 0; // flag de fim da transmicao
char ifName[100]; // nome da interface de rede
int flag=0;

#define DESTMAC0	0x08
#define DESTMAC1	0x00
#define DESTMAC2	0x27
#define DESTMAC3	0x56
#define DESTMAC4	0x75
#define DESTMAC5	0x1A

#define destination_ip "10.0.2.15"

int total_len = 0, send_len = 0,tam=0; 

unsigned short in_cksum(unsigned short *addr,int len) //checksum do cabeçalho implementado
{
        register int sum = 0;
        u_short answer = 0;
        register u_short *w = addr;
        register int nleft = len;

        /*
         * Our algorithm is simple, using a 32 bit accumulator (sum), we add
         * sequential 16 bit words to it, and at the end, fold back all the
         * carry bits from the top 16 bits into the lower 16 bits.
         */
        while (nleft > 1)  {
                sum += *w++;
                nleft -= 2;
        }

        /* mop up an odd byte, if necessary */
        if (nleft == 1) {
                *(u_char *)(&answer) = *(u_char *)w ;
                sum += answer;
        }

        /* add back carry outs from top 16 bits to low 16 bits */
        sum = (sum >> 16) + (sum & 0xffff);     /* add hi 16 to low 16 */
        sum += (sum >> 16);                     /* add carry */
        answer = ~sum;                          /* truncate to 16 bits */
        return(answer);
}

void get_eth_index()  // funcao pra pegar o indice da interface
{
	memset(&ifreq_i,0,sizeof(ifreq_i));
	strncpy(ifreq_i.ifr_name,ifName,IFNAMSIZ-1);

	if((ioctl(sock_raw,SIOCGIFINDEX,&ifreq_i))<0)
		printf("error in index ioctl reading");

	printf("index=%d\n",ifreq_i.ifr_ifindex);

}

void get_mac() // funcao pra pegar o mac da maquina local
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

void get_data() // funcao que monta o cabecalho e os dados a serem enviados
{
  int c,aux;
	char arq[512];
	/*|Numseq|TAM 2 Bytes|FLAGS|*/
	//monta o cabecalho
	printf("\tnumseq=%d\n",numseq);
	cab.numseq=htons(numseq++);
	printf("\tnumseq=%X\n",ntohs(cab.numseq));
	printf("\tnumseq=%X\n",cab.numseq);
	cab.flags=htons(0x0000);
	printf("\tflags=%X\n",cab.flags);
	printf("len = %d\n",total_len);
	c = fread (arq, sizeof(char), 512, pFile); // le o arquivo
	//total_len=c;
	//printf("Arq= %s\n",arq);
	printf("lido %d bytes\n",c);
	//preenche o padding e a flag de fim
	if(c<512){
		//printf("\taqui9\n");
		endFileTransmission = 1;
		cab.flags=htons(0x0001);
		flag=1;
		for(aux=c;aux<512;aux++){
			arq[aux]='A';
		}
	}/**/
	cab.tam=htons(c);
	printf("tam %d \n",ntohs(cab.tam));
	printf("tam %X \n",cab.tam);
	cab.checksum=htons(in_cksum((unsigned short*)arq,512));	
	printf("check %X \n",ntohs(cab.checksum));
	memcpy(sendbuff+total_len, &cab,sizeof(cab));//monta o cabecalho pra envio
	total_len+=sizeof(cab);
	memcpy(sendbuff+total_len, arq,512);//monta os dados pra envio
	total_len+=512;
	//printf("\taqui8\n");
	//printf("%s \n",sendbuff);

}

void get_udp() // funcao que monta o cabecalho UDP
{
  udph->source	= htons(5001);
	udph->dest	= htons(5002);
	udph->check	= 0;

	total_len+= sizeof(struct udphdr);
	get_data();
	udph->len		= htons((total_len - sizeof(struct iphdr) - sizeof(struct ethhdr)));

}

unsigned short checksum(unsigned short* buff, int _16bitword)// checksum do cabecalho IP
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


void get_ip()// funcao que monta o cabecaho IP
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

int recebe(){ // funcao que recebe o ACK
	unsigned char *data;
		int stopReceive;
		uint16_t seq=0;
		struct ethhdr *eth,*ethR;
		struct cabecalho *cbl;
		unsigned char *aux;



    if((sockd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) < 0) {
       printf("Erro na criacao do socket.\n");
    }
	// O procedimento abaixo eh utilizado para "setar" a interface em modo promiscuo
	//strcpy(ifr.ifr_name, "eth0");s
	strcpy(ifr.ifr_name, ifName);
	if(ioctl(sockd, SIOCGIFINDEX, &ifr) < 0)
		printf("erro no ioctl!");
	
	ioctl(sockd, SIOCGIFFLAGS, &ifr);
	ifr.ifr_flags |= IFF_PROMISC;
	ioctl(sockd, SIOCSIFFLAGS, &ifr);/**/
	// laco de repeticao pra pegar os pacotes de ack 
	while(1){
			printf("lendo\n");
			memset(&buff1, 0, sizeof(buff1));
   		tam=recvfrom(sockd,(char *) &buff1, sizeof(buff1), 0x0, NULL, NULL);
			//recv(sockd,&recev, sizeof(recev), 0x0);
			ethR = (struct ethhdr *)(buff1); // pega cabechao ethernet
			iphR = (struct iphdr*)(buff1 + sizeof(struct ethhdr));//pega o cabecalho IP
  		udphR = (struct udphdr *)(buff1 + sizeof(struct iphdr) + sizeof(struct ethhdr));//pega o cabecalho UDP
			cbl=	(struct cabecalho *)(buff1 + sizeof(struct iphdr) + sizeof(struct ethhdr) + sizeof(struct udphdr));// pega cabecalho criado
			data=(buff1 + sizeof(struct iphdr) + sizeof(struct ethhdr) + sizeof(struct udphdr)+sizeof(struct cabecalho));//pega os dados
			printf("MAC Origem: %x:%x:%x:%x:%x:%x \n", ethR->h_source[0],ethR->h_source[1],ethR->h_source[2],ethR->h_source[3],ethR->h_source[4],ethR->h_source[5]);
			//printf("MAC Origem:  %x:%x:%x:%x:%x:%x \n\n", buff1[6],buff1[7],buff1[8],buff1[9],buff1[10],buff1[11]);
			printf("MAC Destino:  %x:%x:%x:%x:%x:%x \n\n", ethR->h_dest[0],ethR->h_dest[1],ethR->h_dest[2],ethR->h_dest[3],ethR->h_dest[4],ethR->h_dest[5]);
			printf("proto = %4X",(ntohs(ethR->h_proto)));
			printf("porta = %d",(ntohs(udphR->dest)));
			if(ethR->h_proto==htons(ETH_P_IP)){ // verifica se e protocolo IP
				//if(iph->daddr[0]== 10 && iph->daddr[1]== 0 && iph->daddr[2]==  2 && iph->daddr[3]==15 ) {
      		if((ntohs(udphR->dest) == 5001)){ //verifica a porta
					// impressão do conteudo - exemplo Endereco Destino e Endereco Origem
						//printf("MAC Destino: %x:%x:%x:%x:%x:%x \n", buff1[0],buff1[1],buff1[2],buff1[3],buff1[4],buff1[5]);
						
						//printf("data= %s\n",data);
						printf("num seq = %d\n",ntohs(cbl->numseq));
						printf("flag = %4X\n",ntohs(cbl->flags));
						printf("tam = %d\n",ntohs(cbl->tam));
					/*	if(ntohs(cbl->flags)==0x0002){
								stopReceive = 1;
								printf("flagfim\n");
						}*///memcmp(&cbl->checksum,&check,sizeof(cbl->checksum))==0
						seq=ntohs(cbl->numseq);
						printf("ack = %d\n",current_ack);
						printf("seq = %d\n",seq);
						if(current_ack==seq){ // verifica se e o ack correto
							current_ack++;
							//current_ack ++;
							printf("ack = %d\n",current_ack);
							int i = 0;
							return 1;
						}else return 0;
			}
			
		}

}

}


int main(int argc, char *argv[])
{
		int controle=1,rec=1,cont=0;
  	pFile=fopen ("README.md","r");

	// pega a interface
	if (argc > 1)
		strcpy(ifName, argv[1]);
	else
		strcpy(ifName, "eth0");
	sock_raw=socket(AF_PACKET,SOCK_RAW,IPPROTO_RAW);
	if(sock_raw == -1)
		printf("error in socket");
	sendbuff=(unsigned char*)malloc(562); // aloca o buffer de envio
  memset(sendbuff,0,562);
 	unsigned char *aux = sendbuff;

  	//inicializando estruturas.
  iph = (struct iphdr*)(sendbuff + sizeof(struct ethhdr));
  udph = (struct udphdr *)(sendbuff + sizeof(struct iphdr) + sizeof(struct ethhdr));


	//pega o mac e a interface
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
	while(!endFileTransmission){
    printf("sending...\n");
		//for para o slowstart
		for(cont=0;cont<controle;cont++){
			//monta o cabecalho IP e envia
		  get_ip();
			printf("enviando\n");
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
			if(flag == 1 ) { 
				controle=cont+1;
				break;
			}
		}
		//recebe todos os ack
		for(cont=0;cont<controle;cont++){
			rec*=recebe();
			printf("rec=%d",rec);
		}
		//controle o slow start
		printf("rec=%d",rec);
		if(rec!=0){
			printf("return1");
			controle*=2;
		}else{
			printf("return0");
			controle=1;
			rec=1;
		}
	}
    fclose (pFile);

}

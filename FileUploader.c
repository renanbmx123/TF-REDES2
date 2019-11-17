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

#include "cabecalho.h"


struct ifreq ifreq_c,ifreq_i,ifreq_ip; /// for each ioctl keep diffrent ifreq structure otherwise error may come in sending(sendto )
struct iphdr *iph;
struct udphdr *udph;
struct cabecalho cab;

int sock_raw;
uint8_t numseq=0;
unsigned char *sendbuff;
FILE *pFile;
char endFileTransmission = 0;
char ifName[100];

#define DESTMAC0	0xd8
#define DESTMAC1	0xfc
#define DESTMAC2	0x93
#define DESTMAC3	0x77
#define DESTMAC4	0xdd
#define DESTMAC5	0xc3

#define destination_ip "10.0.2.15"

int total_len = 0, send_len = 0;

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
	printf("\tnumseq=%d\n",numseq);
	cab.numseq=htons(numseq++);
	printf("\tnumseq=%X\n",ntohs(cab.numseq));
	printf("\tnumseq=%X\n",cab.numseq);
	cab.flags=htons(0x0000);
	printf("\tflags=%X\n",cab.flags);
	printf("len = %d\n",total_len);
	c = fread (arq, sizeof(char), 512, pFile);
	//total_len=c;
	printf("Arq= %s\n",arq);
	printf("lido %d bytes\n",c);
	if(c<512){
		printf("\taqui9\n");
		endFileTransmission = 1;
		cab.flags=htons(0x0001);
	}/**/
	cab.tam=htons(c);
	printf("tam %d \n",ntohs(cab.tam));
	printf("tam %X \n",cab.tam);
	memcpy(sendbuff+total_len, &cab,sizeof(cab));
	total_len+=sizeof(cab);
	memcpy(sendbuff+total_len, arq,c);
	total_len+=c;
	printf("\taqui8\n");
	printf("%s \n",sendbuff);

}

void get_udp()
{
  udph->source	= htons(5001);
	udph->dest	= htons(5002);
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
  //printf("tamanho: %d\n",htonl(iph->tot_len));
	iph->check = 0;
	iph->check	= htons(checksum((unsigned short*)(sendbuff + sizeof(struct ethhdr)), (sizeof(struct iphdr)/2)));

}

int main(int argc, char *argv[])
{
  	pFile=fopen ("README.md","r");


	if (argc > 1)
		strcpy(ifName, argv[1]);
	else
		strcpy(ifName, "eth0");
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
	while(!endFileTransmission){
    printf("sending...\n");

    get_ip();

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
	}
    fclose (pFile);

}

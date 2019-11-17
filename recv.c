#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#include <arpa/inet.h>        // inet_pton() and inet_ntop()

#include <netinet/ip.h>       // struct ip6_hdr
#include <netinet/udp.h>      // struct tcphdr
#include <netinet/if_ether.h> // struct ehternet

// Libraries for setting interface in promiscuous mode!! ifreq
#include <net/if.h>
#include <sys/ioctl.h>

#include "cabecalho.h"

// #define ifName "enp7s0"
#define ETHERTYPE_IPV4 0x08000

#define DESTMAC0	0xd8
#define DESTMAC1	0xfc
#define DESTMAC2	0x93
#define DESTMAC3	0x77
#define DESTMAC4	0xdd
#define DESTMAC5	0xc3

char dst[] ={0xd8,0xfc,0x93,0x77,0xdd,0xc3};
char src[] ={0x80,0x86,0xF2,0xF1,0x30,0x4C};

int iphdrlen, sock_r;
uint16_t current_ack = 0;
FILE *pFile;
char ifName[100];
unsigned char * data;
char stopReceive = 0;

struct sockaddr saddr;
struct sockaddr_in source,dest;
struct ifreq ifreq_c,ifreq_i,ifreq_ip; /// for each ioctl keep diffrent ifreq structure otherwise error may come in sending(sendto )

struct iphdr *ip;
struct ethhdr *eth;
struct udphdr *udp;

struct iphdr *send_ip;
struct ethhdr *send_eth;
struct udphdr *send_udp;

// Funçoes Auxiliares
void get_eth_index(){
	memset(&ifreq_i,0,sizeof(ifreq_i));
	strncpy(ifreq_i.ifr_name,ifName,IFNAMSIZ-1);

	if((ioctl(sock_r,SIOCGIFINDEX,&ifreq_i))<0)
		printf("error in index ioctl reading");

	printf("index=%d\n",ifreq_i.ifr_ifindex);

}

void get_mac()
{
	memset(&ifreq_c,0,sizeof(ifreq_c));
	strncpy(ifreq_c.ifr_name,ifName,IFNAMSIZ-1);

	if((ioctl(sock_r,SIOCGIFHWADDR,&ifreq_c))<0)
		printf("error in SIOCGIFHWADDR ioctl reading");
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


void ethernet_header(unsigned char* buffer,int buflen, int sel)
{
		if (sel == 1) {
			eth = (struct ethhdr *)(buffer);
		} else
		{
			get_mac(sock_r);
			printf("ethernet packaging start ... \n");

			send_eth = (struct ethhdr *)(buffer);
		  send_eth->h_source[0] = (unsigned char)(ifreq_c.ifr_hwaddr.sa_data[0]);
		  send_eth->h_source[1] = (unsigned char)(ifreq_c.ifr_hwaddr.sa_data[1]);
		  send_eth->h_source[2] = (unsigned char)(ifreq_c.ifr_hwaddr.sa_data[2]);
		  send_eth->h_source[3] = (unsigned char)(ifreq_c.ifr_hwaddr.sa_data[3]);
		  send_eth->h_source[4] = (unsigned char)(ifreq_c.ifr_hwaddr.sa_data[4]);
		  send_eth->h_source[5] = (unsigned char)(ifreq_c.ifr_hwaddr.sa_data[5]);

			memcpy(send_eth->h_dest,eth->h_source,sizeof(eth->h_source));
		  eth->h_proto = htons(ETH_P_IP);   //0x800
		  printf("ethernet packaging done.\n");
			printf("Mac= %.2X-%.2X-%.2X-%.2X-%.2X-%.2X\n",(unsigned char)(ifreq_c.ifr_hwaddr.sa_data[0]),(unsigned char)(ifreq_c.ifr_hwaddr.sa_data[1]),(unsigned char)(ifreq_c.ifr_hwaddr.sa_data[2]),(unsigned char)(ifreq_c.ifr_hwaddr.sa_data[3]),(unsigned char)(ifreq_c.ifr_hwaddr.sa_data[4]), 0x44);

			buflen+=sizeof(struct ethhdr);
		}
}

void ip_header(unsigned char* buffer,int buflen, int sel)
{
	if(sel == 1){
		ip = (struct iphdr*)(buffer + sizeof(struct ethhdr));

		iphdrlen =ip->ihl*4;

		memset(&source, 0, sizeof(source));
		source.sin_addr.s_addr = ip->saddr;
		memset(&dest, 0, sizeof(dest));
		dest.sin_addr.s_addr = ip->daddr;
	}else{
		memset(&ifreq_ip,0,sizeof(ifreq_ip));
		strncpy(ifreq_ip.ifr_name, ifName,IFNAMSIZ-1);
	  if(ioctl(sock_r,SIOCGIFADDR,&ifreq_ip)<0)
	 	 {
			printf("error in SIOCGIFADDR \n");
		 }

	 	printf("%s\n",inet_ntoa((((struct sockaddr_in*)&(ifreq_ip.ifr_addr))->sin_addr)));
		send_ip = (struct iphdr*)(buffer + sizeof(struct ethhdr));
		send_ip->ihl	= 5;
	 	send_ip->version	= 4;
	 	send_ip->tos	= 16;
	 	send_ip->id		= htons(10201);
	 	send_ip->ttl	= 64;
	 	send_ip->protocol	= 17;
	 	send_ip->saddr	= inet_addr(inet_ntoa((((struct sockaddr_in *)&(ifreq_ip.ifr_addr))->sin_addr)));
	 	memcpy(&send_ip->daddr , inet_ntoa(source.sin_addr), sizeof(source.sin_addr)); // put destination IP address
	 	buflen += sizeof(struct iphdr);
	}

  // fprintf(pFile , "\nIP Header\n");
}

void udp_header(unsigned char* buffer, int buflen, int sel)
{
	ethernet_header(buffer,buflen, sel);
	ip_header(buffer,buflen, sel);
	if(sel == 1){
		udp = (struct udphdr*)(buffer + iphdrlen + sizeof(struct ethhdr));
	}else {
		send_udp = (struct udphdr *)(buffer + sizeof(struct iphdr) + sizeof(struct ethhdr));

		send_udp->source	= htons(udp->dest);
		send_udp->dest	= htons(udp->source);
		send_udp->check	= 0;
		buflen+= sizeof(struct udphdr);
		send_udp->len		= htons((buflen - sizeof(struct iphdr) - sizeof(struct ethhdr)));

		send_ip->tot_len	= htons(buflen - sizeof(struct ethhdr));
		send_ip->check	= htons(checksum((unsigned short*)(buffer + sizeof(struct ethhdr)), (sizeof(struct iphdr)/2)));
	}
}

void data_process(unsigned char* buffer,int buflen)
{
	unsigned char* send_buffer = (unsigned char *)malloc(65536);
	int send_len = 0;
	memset(send_buffer,0,65536);

	struct iphdr *ip = (struct iphdr*)(buffer + sizeof (struct ethhdr));
  udp_header(buffer, buflen, 1);
  if(ip->protocol == 17){
    if(!memcmp(inet_ntoa(dest.sin_addr),"192.168.0.188",sizeof(dest.sin_addr))) {
      if((ntohs(udp->dest) == 5002)){
				// Grab file parts
				struct cabecalho *cab = (struct cabecalho*)(buffer + iphdrlen  + sizeof(struct ethhdr) + sizeof(struct udphdr));

				if(current_ack == htons(cab->numseq)){
						current_ack = htons(cab->numseq);
						printf("%ld\n",current_ack );
						int i = 0;
	        	unsigned char * data = (buffer + iphdrlen  + sizeof(struct ethhdr) + sizeof(struct udphdr));
	        	int remaining_data = buflen - (iphdrlen  + sizeof(struct ethhdr) + sizeof(struct udphdr));
	        	for(i=6;i<remaining_data;i++)
	        	{
	          	putc(data[i], pFile);
	        	}
				}else {
					udp_header(send_buffer, send_len, 0);
					printf("packet loss\n");
				}
				// stopReceive = 1;
      }

	   }
   }
}

int main(int argc, char *argv[])
{

	int saddr_len,buflen;

	unsigned char* buffer = (unsigned char *)malloc(65536);
	memset(buffer,0,65536);

  if (argc > 1)
      strcpy(ifName, argv[1]);
    else
    strcpy(ifName, "eth0");

	pFile=fopen("RECEBIDO.md","w");
	if(!pFile)
	{
		printf("unable to open log.txt\n");
		return -1;

	}

	printf("starting .... \n");

	sock_r=socket(AF_PACKET,sock_r,htons(ETH_P_ALL));
	if(sock_r<0)
	{
		printf("error in socket\n");
		return -1;
	}

	while(!stopReceive)
	{
		saddr_len=sizeof saddr;
		buflen=recvfrom(sock_r,buffer,65536,0,&saddr,(socklen_t *)&saddr_len);


		if(buflen<0)
		{
			printf("error in reading recvfrom function\n");
			return -1;
		}
		fflush(pFile);
		data_process(buffer,buflen);

	}
	fclose(pFile);
	printf("DONE!!!!\n");

}

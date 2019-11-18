#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/socket.h>

#include <errno.h>		// errno, perror()


#include <arpa/inet.h>        // inet_pton() and inet_ntop()

#include <netinet/ip.h>       // struct ip6_hdr
#include <netinet/udp.h>      // struct tcphdr
#include <netinet/if_ether.h> // struct ehternet
#include <netinet/tcp.h>				/* the names on other systems are easy to guess.. */


// Libraries for setting interface in promiscuous mode!! ifreq
#include <net/if.h>
#include <sys/ioctl.h>

#include "cabecalho.h"

// #define ifName "enp7s0"
#define ETHERTYPE_IPV4 0x08000

//#define DEBUG

#define DESTMAC0	0xd8
#define DESTMAC1	0xfc
#define DESTMAC2	0x93
#define DESTMAC3	0x77
#define DESTMAC4	0xdd
#define DESTMAC5	0xc3

#define destination_ip "192.168.0.188" ////"10.0.2.15"

char dst[] ={0xd8,0xfc,0x93,0x77,0xdd,0xc3};
char src[] ={0x80,0x86,0xF2,0xF1,0x30,0x4C};

int iphdrlen, sock_r, sendLen = 0, buflen = 0;
uint16_t current_ack = 0;
FILE *pFile;
char ifName[100];
unsigned char * data;
unsigned char* buffer;
unsigned char* send_buffer;
char stopReceive = 0;

struct sockaddr saddr;
struct sockaddr_ll sadr_ll;

struct sockaddr_in source,dest;
struct ifreq ifreq_c,ifreq_i,ifreq_ip; /// for each ioctl keep diffrent ifreq structure otherwise error may come in sending(sendto )

struct iphdr  *ip;
struct ethhdr *eth;
struct udphdr *udp;

struct iphdr *send_ip;
struct ethhdr *send_eth;
struct udphdr *send_udp;
struct cabecalho *cab;
// Funçoes Auxiliares
void get_eth_index(){
	memset(&ifreq_i,0,sizeof(ifreq_i));
	strncpy(ifreq_i.ifr_name,ifName,IFNAMSIZ-1);

	if((ioctl(sock_r,SIOCGIFINDEX,&ifreq_i))<0)
		printf("error in index ioctl reading");

	printf("index=%d\n",ifreq_i.ifr_ifindex);

}

void get_mac(){
	memset(&ifreq_c,0,sizeof(ifreq_c));
	strncpy(ifreq_c.ifr_name,ifName,IFNAMSIZ-1);

	if((ioctl(sock_r,SIOCGIFHWADDR,&ifreq_c))<0)
		printf("error in SIOCGIFHWADDR ioctl reading");
}


unsigned short checksum(unsigned short* buff, int _16bitword){
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


void ethernet_header(){
			eth = (struct ethhdr *)(buffer);
}
void get_data(){
	cab = (struct cabecalho *)(send_buffer+sendLen);
	cab->tam=htons(0x0000);
	cab->flags=htons(0x0002);
	cab->numseq=htons(current_ack);
	#if defined(DEBUG)
	printf("\tflags=%X\n",cab->flags);
	printf("len = %d\n",sendLen);
	/*|Numseq|TAM 2 Bytes|FLAGS|*/
	printf("\tack=%d\n",current_ack);
	printf("\tnumseq=%X\n",ntohs(cab->numseq));
	printf("tam %X \n",cab->tam);
	// memcpy(buffer+buflen, &cab,sizeof(cab));
	#endif
	sendLen+=sizeof(struct cabecalho);
}
void ip_header(){
		ip = (struct iphdr*)(buffer + sizeof(struct ethhdr));
		iphdrlen = ip->ihl*4;
		memset(&source, 0, sizeof(source));
		source.sin_addr.s_addr = ip->saddr;
		memset(&dest, 0, sizeof(dest));
		dest.sin_addr.s_addr = ip->daddr;
}

void udp_header(){
	ethernet_header();
	ip_header();
	udp = (struct udphdr*)(buffer + iphdrlen + sizeof(struct ethhdr));
}

void sendFrame(){
	/* Ethernet header */
	/*
	**	Mounting ethernet package for sending
	*/
	send_eth = (struct ethhdr *)(buffer);
	// Set source mac
	memcpy(send_eth->h_source, ifreq_c.ifr_hwaddr.sa_data, sizeof(ifreq_c.ifr_hwaddr.sa_data));
		// Set dest mac address.
	memcpy(send_eth->h_dest,eth->h_source,sizeof(eth->h_dest));
	send_eth->h_proto = htons(ETH_P_IP);   //0x800h
	sendLen+=sizeof(struct ethhdr);
	#if defined(DEBUG)
	printf("\nethernet packaging start ... \n");
	printf("SMac= %.2X-%.2X-%.2X-%.2X-%.2X-%.2X\n",send_eth->h_source[0],send_eth->h_source[1],send_eth->h_source[2],send_eth->h_source[3],send_eth->h_source[4],send_eth->h_source[5]);
	printf("DMac= %.2X-%.2X-%.2X-%.2X-%.2X-%.2X\n",send_eth->h_dest[0], send_eth->h_dest[1], send_eth->h_dest[2], send_eth->h_dest[3],send_eth->h_dest[4],send_eth->h_dest[5]);
	printf("Type: %d\n", send_eth->h_proto);
	printf("ethernet packaging done.\n");
	#endif
	/* IP UDP header */
	/*
	**	Mounting ip and upd package for sending
	*/
	memset(&ifreq_ip,0,sizeof(ifreq_ip));
	strncpy(ifreq_ip.ifr_name, ifName,IFNAMSIZ-1);	// Grab current my own ip
	if(ioctl(sock_r,SIOCGIFADDR,&ifreq_ip)<0)
		printf("error in SIOCGIFADDR \n");

	send_ip = (struct iphdr*)(send_buffer + sizeof(struct ethhdr));
	send_ip->version	= 4;
	send_ip->ihl			= 5;
	send_ip->tos			= 16;
	send_ip->id			  = htons(10201);
	send_ip->ttl	    = 255;
	send_ip->protocol	= 17;
	send_ip->tot_len  =  sizeof (struct ip) + sizeof (struct tcphdr);
	send_ip->check	  = 0;
	send_ip->saddr	  = inet_addr(inet_ntoa((((struct sockaddr_in *)&(ifreq_ip.ifr_addr))->sin_addr)));
	memcpy(&send_ip->daddr , inet_ntoa(source.sin_addr), sizeof(source.sin_addr)); // put destination IP address
	sendLen += sizeof(struct iphdr);

	send_udp = (struct udphdr *)(send_buffer + sizeof(struct iphdr) + sizeof(struct ethhdr));
	send_udp->source	= ntohs(udp->dest);
	send_udp->dest	= ntohs(udp->source);
	send_udp->check	= 0;
	send_udp->len		= ntohs((sendLen - sizeof(struct iphdr) - sizeof(struct ethhdr)));
	send_ip->tot_len	= ntohs(sendLen - sizeof(struct ethhdr));
	sendLen+= sizeof(struct udphdr);
	iphdrlen = ip->ihl*4;

	get_data();
	send_ip->check	= ntohs(checksum((unsigned short*)(send_buffer + sizeof(struct ethhdr)), (sizeof(struct iphdr)/2)));

	#if defined(DEBUG)
	printf("\nIP Header\n");
	printf("\t|-Version              : %d\n",(unsigned int)send_ip->version);
	printf("\t|-Internet Header Length  : %d DWORDS or %d Bytes\n",(unsigned int)send_ip->ihl,((unsigned int)(send_ip->ihl))*4);
	printf("\t|-Type Of Service   : %d\n",(unsigned int)send_ip->tos);
	printf("\t|-Total Length      : %d  Bytes\n",ntohs(send_ip->tot_len));
	printf("\t|-Identification    : %d\n",ntohs(send_ip->id));
	printf("\t|-Time To Live	    : %d\n",(unsigned int)send_ip->ttl);
	printf("\t|-Protocol 	    : %d\n",(unsigned int)send_ip->protocol);
	printf("\t|-Header Checksum   : %d\n",ntohs(send_ip->check));
	printf("\t|-Source IP         : %s\n", inet_ntoa(source.sin_addr));
	printf("\t|-Destination IP    : %s\n",inet_ntoa(dest.sin_addr));
	printf("\nUDP Header\n");
	printf("\t|-Source Port    	: %d\n" , send_udp->source);
	printf("\t|-Destination Port	: %d\n" , send_udp->dest);
	printf("\t|-UDP Length      	: %d\n" , send_udp->len);
	printf("\t|-UDP Checksum   	: %d\n" , send_udp->check);
  #endif
}

int data_process(){
	struct iphdr *ip = (struct iphdr*)(buffer + sizeof (struct ethhdr));
	iphdrlen = 0;
  udp_header();
  if(ip->protocol == 17){
    if(!memcmp(inet_ntoa(dest.sin_addr), destination_ip, sizeof(dest.sin_addr))) {
      if((ntohs(udp->dest) == 5002)){
				// Grab file parts
				struct cabecalho *cab = (struct cabecalho*)(buffer + iphdrlen  + sizeof(struct ethhdr) + sizeof(struct udphdr));

				if(ntohs(cab->flags)==0x0001){
						stopReceive = 1;
				}
				if(current_ack == ntohs(cab->numseq)){
						current_ack = ntohs((cab->numseq));
						current_ack ++;
						unsigned char * data = (send_buffer + iphdrlen  + sizeof(struct ethhdr) + sizeof(struct udphdr));
						int remaining_data = buflen - (iphdrlen  + sizeof(struct ethhdr) + sizeof(struct udphdr));

						for(int i=0;i < remaining_data;i++){
							putc(data[i], pFile);
							printf("%.2X",data[i]);
						}
						sadr_ll.sll_ifindex = ifreq_i.ifr_ifindex;
						// Create udp stack with ack message
						memset(send_buffer,0,sizeof(send_buffer));
						sendLen = 0;
						sendFrame();
						int i = 0;
						sadr_ll.sll_halen   = ETH_ALEN;
						memcpy(sadr_ll.sll_addr,eth->h_source,sizeof(eth->h_source));

						#if defined(DEBUG)
							printf("\n%d", sadr_ll.sll_ifindex );
							printf("\n%d\n", sadr_ll.sll_halen);
							printf("%.2X-%.2X-%.2X-%.2X-%.2X-%.2X\n", (sadr_ll.sll_addr[0]), (sadr_ll.sll_addr[1]), (sadr_ll.sll_addr[2]), (sadr_ll.sll_addr[3]),(sadr_ll.sll_addr[4]),(sadr_ll.sll_addr[5]));
							printf("%d\n",sendLen );
						#endif
													// socket, data, data_size, flags, addr, addr_size
						int len=sendto(sock_r, send_buffer, sendLen, 0, (const struct sockaddr*)&sadr_ll,sizeof(struct sockaddr_ll));
						if(len<0)
						{
							printf("error in sendto function\n");
							perror(&errno);
							return -1;
						}else{
							printf("ack send\n" );
						}

				}else{
					printf("aqui\n");
					printf("packet loss\n");
				}
				// stopReceive = 1;
      }
	   }
   }
}

int main(int argc, char *argv[]){

	struct ifreq ifopts;
	buffer = (unsigned char *)malloc(65536);
	memset(buffer,0,65536);
	send_buffer = (unsigned char *)malloc(65536);
	memset(send_buffer,0,65536);

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

	sock_r=socket(PF_PACKET,SOCK_RAW,htons(ETH_P_ALL));
	if(sock_r<0)
	{
		printf("error in socket\n");
		return -1;
	}
	/* Set interface to promiscuous mode */
	strncpy(ifopts.ifr_name, ifName, IFNAMSIZ-1);
	ioctl(sock_r, SIOCGIFFLAGS, &ifopts);
	 ifopts.ifr_flags |= IFF_PROMISC;
		ioctl(sock_r, SIOCSIFFLAGS, &ifopts);
	get_mac();
	get_eth_index();

	while(!stopReceive)
	{
		buflen=recvfrom(sock_r,buffer,65536,0, NULL ,0);
		if(buflen<0){
			printf("error in reading recvfrom function\n");
			return -1;
		}

		fflush(pFile);
		data_process();//send_buffer, send_len
	}
	fclose(pFile);
	printf("DONE!!!!\n");

}

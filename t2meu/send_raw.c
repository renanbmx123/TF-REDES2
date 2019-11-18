#include <arpa/inet.h>
#include <linux/if_packet.h>
#include <linux/ip.h>
#include <linux/udp.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/ether.h>
#include "raw.h"
#include <string.h>

#define PROTO_UDP	17
#define DST_PORT	8000

char this_mac[6];
char bcast_mac[6] =	{0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
char dst_mac[6] =	{0x08, 0x00, 0x27, 0x56, 0x75, 0x1a};
char src_mac[6] =	{0x08, 0x00, 0x27, 0x56, 0x75, 0x1a};
FILE *pFile;
char endFileTransmission = 0;


union eth_buffer buffer_u;

void clearMSG(char msg[]){
	int c=0;
	for (c=0;c<1000;c++){
			msg[c]='\0';
		}	
}

uint32_t ipchksum(uint8_t *packet)
{
	uint32_t sum=0;
	uint16_t i;

	for(i = 0; i < 20; i += 2)
		sum += ((uint32_t)packet[i] << 8) | (uint32_t)packet[i + 1];
	while (sum & 0xffff0000)
		sum = (sum & 0xffff) + (sum >> 16);
	return sum;
}

int main(int argc, char *argv[])
{
	struct ifreq if_idx, if_mac, ifopts;
	char ifName[IFNAMSIZ];
	char *p;
	struct sockaddr_ll socket_address;
	struct cabecalho cab;
	int sockfd, numbytes;
	uint8_t msg[1000];	
	uint16_t auxport;
	int c,i,numseq=0;
	//user u;

	/* Get interface name */
	if (argc > 1)
		strcpy(ifName, argv[1]);
	else
		strcpy(ifName, DEFAULT_IF);

	/* Open RAW socket */
	if ((sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) == -1)
		perror("socket");

	/* Set interface to promiscuous mode */
	/*strncpy(ifopts.ifr_name, ifName, IFNAMSIZ-1);
	ioctl(sockfd, SIOCGIFFLAGS, &ifopts);
	ifopts.ifr_flags |= IFF_PROMISC;
	ioctl(sockfd, SIOCSIFFLAGS, &ifopts);*/
	//scanf("%[^\n]s",msg);

	/* Get the index of the interface */
	memset(&if_idx, 0, sizeof(struct ifreq));
	strncpy(if_idx.ifr_name, ifName, IFNAMSIZ-1);
	if (ioctl(sockfd, SIOCGIFINDEX, &if_idx) < 0)
		perror("SIOCGIFINDEX");
	socket_address.sll_ifindex = if_idx.ifr_ifindex;
	socket_address.sll_halen = ETH_ALEN;

	/* Get the MAC address of the interface */
	memset(&if_mac, 0, sizeof(struct ifreq));
	strncpy(if_mac.ifr_name, ifName, IFNAMSIZ-1);
	if (ioctl(sockfd, SIOCGIFHWADDR, &if_mac) < 0)
		perror("SIOCGIFHWADDR");
	memcpy(this_mac, if_mac.ifr_hwaddr.sa_data, 6);
	pFile=fopen ("README.md","r");
	/* End of configuration. Now we can send data using raw sockets. */
	while(!endFileTransmission){
		/*|Numseq|TAM 2 Bytes|FLAGS|*/
		printf("\tnumseq=%d\n",numseq);
		cab.numseq=htons(numseq++);
		printf("\tnumseq=%04X\n",ntohs(cab.numseq));
		printf("\tnumseq=%04X\n",cab.numseq);
		cab.flags=htons(0x0000);
		printf("\tflags=%04X\n",cab.flags);
		c = fread (cab.msg, sizeof(char), 512, pFile);
		if(c<512){
			printf("\taqui9\n");
			endFileTransmission = 1;
			cab.flags=htons(0x0001);
			for(i=c;i<512;i++){
					cab.msg[i]='A';
			}
		}/**/
		cab.tam=htons(c);
		printf("tam %d \n",ntohs(cab.tam));
		printf("tam %04X \n",cab.tam);
		
		printf("sizecab =  %li\n", sizeof(cab));
		printf("siziep =  %li\n", sizeof(struct ip_hdr));
		printf("sizeudp =  %li\n", sizeof(struct udp_hdr));
		/* Fill the Ethernet frame header */
		memcpy(buffer_u.cooked_data.ethernet.dst_addr, bcast_mac, 6);
		memcpy(buffer_u.cooked_data.ethernet.src_addr, src_mac, 6);
		buffer_u.cooked_data.ethernet.eth_type = htons(ETH_P_IP);

		/* Fill IP header data. Fill all fields and a zeroed CRC field, then update the CRC! */
		buffer_u.cooked_data.payload.ip.ver = 0x45;
		buffer_u.cooked_data.payload.ip.tos = 0x00;
		buffer_u.cooked_data.payload.ip.len = htons(sizeof(struct ip_hdr) + sizeof(struct udp_hdr) + sizeof(struct cabecalho));
		printf("iplenA - %d\n", ntohs(buffer_u.cooked_data.payload.ip.len));
		buffer_u.cooked_data.payload.ip.id = htons(0x00);
		buffer_u.cooked_data.payload.ip.off = htons(0x00);
		buffer_u.cooked_data.payload.ip.ttl = 50;
		buffer_u.cooked_data.payload.ip.proto = 17; //0xff;
		buffer_u.cooked_data.payload.ip.sum = htons(0x0000);

		buffer_u.cooked_data.payload.ip.src[0] = 10;
		buffer_u.cooked_data.payload.ip.src[1] = 0;
		buffer_u.cooked_data.payload.ip.src[2] = 2;
		buffer_u.cooked_data.payload.ip.src[3] = 15;
		buffer_u.cooked_data.payload.ip.dst[0] = 10;
		buffer_u.cooked_data.payload.ip.dst[1] = 0;
		buffer_u.cooked_data.payload.ip.dst[2] = 2;
		buffer_u.cooked_data.payload.ip.dst[3] = 15;
		buffer_u.cooked_data.payload.ip.sum = htons((~ipchksum((uint8_t *)&buffer_u.cooked_data.payload.ip) & 0xffff));

		/* Fill UDP header */
		buffer_u.cooked_data.payload.udp.udphdr.src_port = htons(8000);
		buffer_u.cooked_data.payload.udp.udphdr.dst_port = htons(8000);
		buffer_u.cooked_data.payload.udp.udphdr.udp_len = htons(sizeof(struct udp_hdr) + sizeof(struct cabecalho));
		buffer_u.cooked_data.payload.udp.udphdr.udp_chksum = 0;	
		printf("UDPlen - %d\n", ntohs(buffer_u.cooked_data.payload.udp.udphdr.udp_len));
		/* Fill UDP payload */
		memcpy(buffer_u.raw_data + sizeof(struct eth_hdr) + sizeof(struct ip_hdr) + sizeof(struct udp_hdr), &cab, sizeof(struct cabecalho));
		//memcpy(buffer_u.raw_data + sizeof(struct eth_hdr) + sizeof(struct ip_hdr) + sizeof(struct udp_hdr), cab.msg, c);//+sizeof(struct cabecalho)

		/* Send it.. */
		memcpy(socket_address.sll_addr, dst_mac, 6);
		if (sendto(sockfd, buffer_u.raw_data, sizeof(struct eth_hdr) + sizeof(struct ip_hdr) + sizeof(struct udp_hdr) + c, 0, (struct sockaddr*)&socket_address, sizeof(struct sockaddr_ll)) < 0)
			printf("Send failed\n");
	
		clearMSG(msg);
	}
	//printf("send\n");
	//recv
	fclose (pFile);
	return 0;
}

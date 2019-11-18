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

#define PROTO_UDP	17
#define DST_PORT	5002

char bcast_mac[6] =	{0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
char dst_mac[6] =	{0x00, 0x00, 0x00, 0x22, 0x22, 0x22};
char src_mac[6] =	{0x00, 0x00, 0x00, 0x33, 0x33, 0x33};

union eth_buffer buffer_u;

int main(int argc, char *argv[])
{
	struct ifreq ifopts, if_mac,if_idx ;
	char ifName[IFNAMSIZ];
	int sockfd, numbytes;
	char *p;
	struct cabecalho cab, scab;
	struct sockaddr_ll socket_address;
	uint8_t msg[1000];	
	uint16_t auxport;
	int pos;
	int c=0;
	char op=' ';
	char canal[30]="";
	/* Get interface name */
	if (argc > 1)
		strcpy(ifName, argv[1]);
	else
		strcpy(ifName, DEFAULT_IF);

	/* Open RAW socket */
	if ((sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) == -1)
		perror("socket");
	
	/* Set interface to promiscuous mode */
	strncpy(ifopts.ifr_name, ifName, IFNAMSIZ-1);
	ioctl(sockfd, SIOCGIFFLAGS, &ifopts);
	ifopts.ifr_flags |= IFF_PROMISC;
	ioctl(sockfd, SIOCSIFFLAGS, &ifopts);

	/* End of configuration. Now we can receive data using raw sockets. */

	while (1){
		/*recv*/
		
		numbytes = recvfrom(sockfd, buffer_u.raw_data, ETH_LEN, 0, NULL, NULL);
		if (buffer_u.cooked_data.ethernet.eth_type == ntohs(ETH_P_IP) ){//&& ((buffer_u.cooked_data.payload.ip.dst[0] == 10)&&(buffer_u.cooked_data.payload.ip.dst[1]==32)&&(buffer_u.cooked_data.payload.ip.dst[2]==143)&&(buffer_u.cooked_data.payload.ip.dst[3]==83)
			/*printf("IP packet, %d bytes - src ip: %d.%d.%d.%d dst ip: %d.%d.%d.%d proto: %d\n",
				numbytes,
				buffer_u.cooked_data.payload.ip.src[0], buffer_u.cooked_data.payload.ip.src[1],
				buffer_u.cooked_data.payload.ip.src[2], buffer_u.cooked_data.payload.ip.src[3],
				buffer_u.cooked_data.payload.ip.dst[0], buffer_u.cooked_data.payload.ip.dst[1],
				buffer_u.cooked_data.payload.ip.dst[2], buffer_u.cooked_data.payload.ip.dst[3],
				buffer_u.cooked_data.payload.ip.proto
			);*/
			if (buffer_u.cooked_data.payload.ip.proto == PROTO_UDP && buffer_u.cooked_data.payload.udp.udphdr.dst_port == ntohs(DST_PORT)){
				p = (char *)&buffer_u.cooked_data.payload.udp.udphdr + ntohs(buffer_u.cooked_data.payload.udp.udphdr.udp_len);
				*p = '\0';
				printf("src port: %d dst port: %d size: %d msg: %s\n", 
				ntohs(buffer_u.cooked_data.payload.udp.udphdr.src_port), ntohs(buffer_u.cooked_data.payload.udp.udphdr.dst_port),
				ntohs(buffer_u.cooked_data.payload.udp.udphdr.udp_len), (char *)&buffer_u.cooked_data.payload.udp.udphdr + sizeof(struct udp_hdr) +sizeof(struct cabecalho));
				memcpy(&cab,&buffer_u.cooked_data.payload.udp.udphdr + sizeof(struct udp_hdr),sizeof(struct cabecalho));
				printf("num seq = %d\n",ntohs(cab.numseq));
				printf("flag = %4X\n",ntohs(cab.flags));
				printf("tam = %d\n",ntohs(cab.tam));
				//printf("mensagem = %s\n",cab->msg);
				if(ntohs(cab.flags)==0x0001){
						//stopReceive = 1;
						printf("flagfim\n");
				}
				//pos=comIP(buffer_u.cooked_data.payload.ip.src);
			
				
	
				/*envio*/
	
				/*auxU=usuarios[pos];
				printf("fimC =%d\n",fimC);
				for(c=0;c<fimC;c++){
					printf("canal=%s=%s\n",canais[c].nome, auxU.canal);
					if(strcmp(canais[c].nome, auxU.canal)==0){
						pos=c;
						printf("canal = %s = %s\n",canais[c].nome, auxU.canal);
					}
				}
				printf("%s",msg);
				//printf("canal: %s",canal);
				if(op == MSG){
					for(c=0;c<canais[c].fimM;c++){
						form(msg,&auxU,PRIVMSG,P);
						memset(&if_idx, 0, sizeof(struct ifreq));
						strncpy(if_idx.ifr_name, ifName, IFNAMSIZ-1);
						if (ioctl(sockfd, SIOCGIFINDEX, &if_idx) < 0)
							perror("SIOCGIFINDEX");
						socket_address.sll_ifindex = if_idx.ifr_ifindex;
						socket_address.sll_halen = ETH_ALEN;

						/* Get the MAC address of the interface */
						/*memset(&if_mac, 0, sizeof(struct ifreq));
						strncpy(if_mac.ifr_name, ifName, IFNAMSIZ-1);
						if (ioctl(sockfd, SIOCGIFHWADDR, &if_mac) < 0)
							perror("SIOCGIFHWADDR");
						memcpy(this_mac, if_mac.ifr_hwaddr.sa_data, 6);*/

						/* End of configuration. Now we can send data using raw sockets. */

						/* Fill the Ethernet frame header */
						/*memcpy(buffer_u.cooked_data.ethernet.dst_addr, canais[pos].membros[c].mac, 6);
						memcpy(buffer_u.cooked_data.ethernet.src_addr, src_mac, 6);
						buffer_u.cooked_data.ethernet.eth_type = htons(ETH_P_IP);

						/* Fill IP header data. Fill all fields and a zeroed CRC field, then update the CRC! */
						/*buffer_u.cooked_data.payload.ip.ver = 0x45;
						buffer_u.cooked_data.payload.ip.tos = 0x00;
						buffer_u.cooked_data.payload.ip.len = htons(sizeof(struct ip_hdr) + sizeof(struct udp_hdr) + strlen(msg));
						buffer_u.cooked_data.payload.ip.id = htons(0x00);
						buffer_u.cooked_data.payload.ip.off = htons(0x00);
						buffer_u.cooked_data.payload.ip.ttl = 50;
						buffer_u.cooked_data.payload.ip.proto = 17; //0xff;
						buffer_u.cooked_data.payload.ip.sum = htons(0x0000);

						buffer_u.cooked_data.payload.ip.src[0] = 10;
						buffer_u.cooked_data.payload.ip.src[1] = 0;
						buffer_u.cooked_data.payload.ip.src[2] = 2;
						buffer_u.cooked_data.payload.ip.src[3] = 15;
						buffer_u.cooked_data.payload.ip.dst[0] = canais[pos].membros[c].ip[0];
						buffer_u.cooked_data.payload.ip.dst[1] = canais[pos].membros[c].ip[1];
						buffer_u.cooked_data.payload.ip.dst[2] = canais[pos].membros[c].ip[2];
						buffer_u.cooked_data.payload.ip.dst[3] = canais[pos].membros[c].ip[3];
						buffer_u.cooked_data.payload.ip.sum = htons((~ipchksum((uint8_t *)&buffer_u.cooked_data.payload.ip) & 0xffff));

						/* Fill UDP header */
						/*auxport=ntohs(buffer_u.cooked_data.payload.udp.udphdr.src_port);
						buffer_u.cooked_data.payload.udp.udphdr.src_port = htons(8000);
						buffer_u.cooked_data.payload.udp.udphdr.dst_port = htons(auxport);
						buffer_u.cooked_data.payload.udp.udphdr.udp_len = htons(sizeof(struct udp_hdr) + strlen(msg));
						buffer_u.cooked_data.payload.udp.udphdr.udp_chksum = 0;

						/* Fill UDP payload */
					/*	memcpy(buffer_u.raw_data + sizeof(struct eth_hdr) + sizeof(struct ip_hdr) + sizeof(struct udp_hdr), msg, strlen(msg));

						/* Send it.. */
						/*memcpy(socket_address.sll_addr, dst_mac, 6);
						if (sendto(sockfd, buffer_u.raw_data, sizeof(struct eth_hdr) + sizeof(struct ip_hdr) + sizeof(struct udp_hdr) + strlen(msg), 0, (struct sockaddr*)&socket_address, sizeof(struct sockaddr_ll)) < 0)
							printf("Send failed\n");

						//printf("send\n");

									//fim echo*/

			}
		}		
				//printf("got a packet, %d bytes\n", numbytes);
	}


	return 0;
}

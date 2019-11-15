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
#include "irc.h"

#define PROTO_UDP	17
#define DST_PORT	8000

char this_mac[6];
char bcast_mac[6] =	{0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
char dst_mac[6] =	{0x08, 0x00, 0x27, 0x56, 0x75, 0x1a};
char src_mac[6] =	{0x08, 0x00, 0x27, 0x56, 0x75, 0x1a};



union eth_buffer buffer_u;

user usuarios[100];
channel canais[100];
int iniU =0; fimU =0; iniC=0; fimC=0;

int comIP(uint8_t ip[]){
	int c=0;
	for(c=0;c<fimU;c++){
		if(usuarios[c].ip[0]==ip[0] && usuarios[c].ip[1]==ip[1] && usuarios[c].ip[2]==ip[2] && usuarios[c].ip[3]==ip[3]){
			return c;
		}
	}
	return -1;
}


void split(char msg[],int s,int op, char canal[], char nick[]){
		int c=0,fim=0,flag=0;
		char aux[100];
		//printf("split %d\n",s);
		for(c=0;c<=s;c++){
			//printf("c = %d",c);
			if(msg[c] != '|'){
				//printf("c=%d f=%d ",c,fim);
				aux[fim]=msg[c];
				//printf("%c ",aux[fim]);
				//printf("M = %c ",msg[c]);
				fim++;
			}else{
				//printf("split %d\n",op);
				aux[fim]='\0';
				switch(op){
					case N:
						//printf("NICK: %s\n",aux);
						if(flag==0){
							strcpy(nick,aux);
							flag++;
						}
						break;
					case C:
						if(flag==0){
							strcpy(canal,aux);
							flag++;
						}
						break;
					case M:
						switch (flag){
							case 0:
								strcpy(canal,aux);
								break;
							case 1:
								strcpy(msg,aux);
								break;
							}
							flag++;
						break;
					case P:
						switch (flag){
							case 0:
								strcpy(canal,aux);
								break;
							case 1:
								strcpy(nick,aux);
								break;
							case 2:
								
								break;
							}
							flag++;
						break;
				}
				//printf("%s\n",aux);
				for(;fim>=0;fim--){	
					aux[fim] = ' ';
				}
				fim=0;
			}
		}
		strcpy(msg,aux);
		//printf("%s\n",msg);
}

char trat(char out[],user *u,char msg[],int s){
	int c=0;
	int i=0;
	char aux[220]="";
	char nick[10]="";
	char canal[10]="";
	//printf("  trat = %c\n",msg[0]);
	switch(msg[0]){
		case NICK:
			for(c=1;c<s;c++){
				aux[c-1]=msg[c];
			}
			split(aux,s,N,canal,nick);
			strcpy(out,aux);
			strcpy(u->nick,nick);
			//printf("#%s\n\t<%s> %s\n",canal,nick,out);
			return NICK;
			break;
		case CREATE:
			for(c=1;c<s;c++){
				aux[c-1]=msg[c];
			}
			split(aux,s,C,canal,nick);
			strcpy(out,aux);
			strcpy(canais[fimC].nome,canal);
			//printf("criando");
			canais[fimC].admin=u;
			canais[fimC].fimM = 0;
			fimC++;
			//printf("%s\n\t<%s> %s\n",canais[fimC-1].nome,canais[fimC-1].admin->nick,out);
			return CREATE;
			break;
		case REMOVE:
			for(c=1;c<s;c++){
				aux[c-1]=msg[c];
			}
			split(aux,s,P,canal,nick);
			strcpy(out,aux);
			return REMOVE;
			break;
		case LIST:
			//printf("list");
			aux[0]=LIST;
			strcat(aux,"\n");
			for(c=0;c<fimC;c++){
				if(strcmp(canais[c].nome,"null")!=0){
					strcat(aux,canais[c].nome);
					strcat(aux,"\n");
				}
			}
			//printf("%s",aux);
			strcpy(out,aux);
			return LIST;
			break;
		case JOIN:
			printf("join\n");
			for(c=1;c<s;c++){
				aux[c-1]=msg[c];
			}
			split(aux,s,C,canal,nick);
			printf("\t ab");
			for(c=0;c<fimC;c++){
				printf("procurando\n");
				if(strcmp(canais[c].nome,canal)==0){
					printf("achou");
					memcpy(&canais[c].membros[canais[c].fimM],u,sizeof(u));
					strcpy(u->canal,canais[c].nome);
					printf("membro = %s\n",canais[c].membros[canais[c].fimM].nick);
					printf(" at ");
					canais[c].fimM++;
					//canais[c].ult->usr=u;
					//printf("%s\n",canais[c].ult->usr->nick);
				}
			}
			//form(msg,u,JOIN,P);			
			strcpy(out,aux);
			return JOIN;
			printf("2%s\n",canal);
			//printf("\t ab");
			//strcpy(out,aux);
			break;
		case PART:
			for(c=1;c<s;c++){
				aux[c-1]=msg[c];
			}
			split(aux,s,P,canal,nick);
			strcpy(out,aux);
			return PART;
			break;
		case NAMES:
			for(c=1;c<s;c++){
				aux[c-1]=msg[c];
			}
			split(aux,s,P,canal,nick);
			aux[0]=NAMES;
			aux[1]='\0';
			strcat(aux,"\n*");
			for(c=0;c<fimC;c++){
				if(strcmp(canais[c].nome,canal)==0){
					strcat(aux, canais[c].admin->nick);
					strcat(aux,"\n");
					for(i=0;i<canais[c].fimM;i++){
						strcat(aux,canais[c].membros[c].nick);
						strcat(aux,"\n");
					}
				}
			}
			strcpy(out,aux);
			return NAMES;
			break;
		case KICK:
			for(c=1;c<s;c++){
				aux[c-1]=msg[c];
			}
			split(aux,s,P,canal,nick);
			strcpy(out,aux);
			return KICK;
			break;
		case PRIVMSG:
			for(c=1;c<s;c++){
				aux[c-1]=msg[c];
			}
			//printf("msg = %s\n",aux);
			split(aux,s,P,canal,nick);
			strcpy(out,aux);
			
			//strcpy(channel,canal);
			//printf("#%s\n\t<%s> %s\n",canal,nick,out);
			return PRIVMSG;
			break;
		case MSG:
			for(c=1;c<s;c++){
				aux[c-1]=msg[c];
			}
			//printf("msg = %s\n",aux);
			split(aux,s,M,canal,nick);
			strcpy(out,aux);
			//strcpy(canal,canal);
			//printf("#%s\n\t<%s> %s\n",canal,nick,out);
			return MSG;
			break;
		case QUIT:
			for(c=1;c<s;c++){
				aux[c-1]=msg[c];
			}
			split(aux,s,M,canal,nick);
			strcpy(out,aux);
			return QUIT;
			break;
	}
	
	/*for (c=1;c<sizeof(sting);c++){
		if(string[c]!="/"){
			if(string[c]!=" "){
				aux=[c]=string[c];
			}else{
				
			}
		}else{
			continue;
		}
	}*/
}
void form(char msg[],user *u, char tipo, int op){
	int c=0;
	char aux[100]=" ";
	switch(op){
		case N:
			aux[0] = tipo;
			strcat(aux,u->nick);
			strcat(aux,"|");
			strcpy(msg,aux);
			break;
		case C:
			aux[0] = tipo;
			printf("aqui = %d e %d -",aux[0],MSG);
			strcat(aux,u->canal);
			strcat(aux,"|");
			strcpy(msg,aux);
			printf("%s\n",msg);
			break;
		case M:
			aux[0] = tipo;
			//printf("aqui = %d e %d -",aux[0],MSG);
			strcat(aux,u->canal);
			strcat(aux,"|");
			strcat(aux,msg);
			//printf("%s\n",aux);
			strcpy(msg,aux);
			break;
		case P:
			aux[0] = tipo;
			//printf("aqui = %d e %d -",aux[0],MSG);
			strcat(aux,u->canal);
			strcat(aux,"|");
			strcat(aux,u->nick);
			strcat(aux,"|");
			strcat(aux,msg);
			//printf("%s\n",aux);
			strcpy(msg,aux);
			break;
		}
		for(c=0;c<100;c++){
				aux[c]='\0';
		}
		//return aux;
}

/*int send(char msg[];user *u){
	
	
}
int recv(char msg[];user *u){
	
	
}*/

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
	struct ifreq ifopts, if_mac,if_idx ;
	char ifName[IFNAMSIZ];
	int sockfd, numbytes;
	char *p;
	struct sockaddr_ll socket_address;
	uint8_t msg[30];	
	uint16_t auxport;
	int pos;
	int c=0;
	char op=' ';
	char canal[30]="";
	user auxU;
	strcpy(auxU.nick,"teste");
	strcpy(auxU.canal,"#teste");
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
				ntohs(buffer_u.cooked_data.payload.udp.udphdr.udp_len), (char *)&buffer_u.cooked_data.payload.udp.udphdr + sizeof(struct udp_hdr)
				);
				pos=comIP(buffer_u.cooked_data.payload.ip.src);
				if(pos==-1){
					usuarios[fimU].ip[0]=buffer_u.cooked_data.payload.ip.src[0];
					usuarios[fimU].ip[1]=buffer_u.cooked_data.payload.ip.src[1];
					usuarios[fimU].ip[2]=buffer_u.cooked_data.payload.ip.src[2];
					usuarios[fimU].ip[3]=buffer_u.cooked_data.payload.ip.src[3];
					memcpy(usuarios[fimU].mac,buffer_u.cooked_data.ethernet.src_addr, 6);
					sprintf(usuarios[fimU].nick,"u%d%d",usuarios[fimU].ip[2],usuarios[fimU].ip[3]);
					pos=fimU;
					fimU++;
				}
				for(c=0;c<fimU;c++){
						printf("nick: %s - canal:%s, IP: %d.%d.%d.%d, MAC: %x:%x:%x:%x:%x:%x\n",usuarios[c].nick,usuarios[c].canal,usuarios[c].ip[0],usuarios[c].ip[1],usuarios[c].ip[2],usuarios[c].ip[3],usuarios[c].mac[0],usuarios[c].mac[1],usuarios[c].mac[2],usuarios[c].mac[3],usuarios[c].mac[4],usuarios[c].mac[5]);
				}
				op=trat(msg,&usuarios[pos],(char *)&buffer_u.cooked_data.payload.udp.udphdr + sizeof(struct udp_hdr),ntohs(buffer_u.cooked_data.payload.udp.udphdr.udp_len));
	
				/*envio*/
	
				auxU=usuarios[pos];
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
						memset(&if_mac, 0, sizeof(struct ifreq));
						strncpy(if_mac.ifr_name, ifName, IFNAMSIZ-1);
						if (ioctl(sockfd, SIOCGIFHWADDR, &if_mac) < 0)
							perror("SIOCGIFHWADDR");
						memcpy(this_mac, if_mac.ifr_hwaddr.sa_data, 6);

						/* End of configuration. Now we can send data using raw sockets. */

						/* Fill the Ethernet frame header */
						memcpy(buffer_u.cooked_data.ethernet.dst_addr, canais[pos].membros[c].mac, 6);
						memcpy(buffer_u.cooked_data.ethernet.src_addr, src_mac, 6);
						buffer_u.cooked_data.ethernet.eth_type = htons(ETH_P_IP);

						/* Fill IP header data. Fill all fields and a zeroed CRC field, then update the CRC! */
						buffer_u.cooked_data.payload.ip.ver = 0x45;
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
						auxport=ntohs(buffer_u.cooked_data.payload.udp.udphdr.src_port);
						buffer_u.cooked_data.payload.udp.udphdr.src_port = htons(8000);
						buffer_u.cooked_data.payload.udp.udphdr.dst_port = htons(auxport);
						buffer_u.cooked_data.payload.udp.udphdr.udp_len = htons(sizeof(struct udp_hdr) + strlen(msg));
						buffer_u.cooked_data.payload.udp.udphdr.udp_chksum = 0;

						/* Fill UDP payload */
						memcpy(buffer_u.raw_data + sizeof(struct eth_hdr) + sizeof(struct ip_hdr) + sizeof(struct udp_hdr), msg, strlen(msg));

						/* Send it.. */
						memcpy(socket_address.sll_addr, dst_mac, 6);
						if (sendto(sockfd, buffer_u.raw_data, sizeof(struct eth_hdr) + sizeof(struct ip_hdr) + sizeof(struct udp_hdr) + strlen(msg), 0, (struct sockaddr*)&socket_address, sizeof(struct sockaddr_ll)) < 0)
							printf("Send failed\n");

						//printf("send\n");

									//fim echo
					}
					
				}else {
					form(msg,&auxU,PRIVMSG,P);
					//echo send			
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

					/* End of configuration. Now we can send data using raw sockets. */

					/* Fill the Ethernet frame header */
					memcpy(buffer_u.cooked_data.ethernet.dst_addr, dst_mac, 6);
					memcpy(buffer_u.cooked_data.ethernet.src_addr, src_mac, 6);
					buffer_u.cooked_data.ethernet.eth_type = htons(ETH_P_IP);

					/* Fill IP header data. Fill all fields and a zeroed CRC field, then update the CRC! */
					buffer_u.cooked_data.payload.ip.ver = 0x45;
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
					buffer_u.cooked_data.payload.ip.dst[0] = 10;
					buffer_u.cooked_data.payload.ip.dst[1] = 0;
					buffer_u.cooked_data.payload.ip.dst[2] = 2;
					buffer_u.cooked_data.payload.ip.dst[3] = 15;
					buffer_u.cooked_data.payload.ip.sum = htons((~ipchksum((uint8_t *)&buffer_u.cooked_data.payload.ip) & 0xffff));

					/* Fill UDP header */
					auxport=ntohs(buffer_u.cooked_data.payload.udp.udphdr.src_port);
					buffer_u.cooked_data.payload.udp.udphdr.src_port = htons(8000);
					buffer_u.cooked_data.payload.udp.udphdr.dst_port = htons(auxport);
					buffer_u.cooked_data.payload.udp.udphdr.udp_len = htons(sizeof(struct udp_hdr) + strlen(msg));
					buffer_u.cooked_data.payload.udp.udphdr.udp_chksum = 0;

					/* Fill UDP payload */
					memcpy(buffer_u.raw_data + sizeof(struct eth_hdr) + sizeof(struct ip_hdr) + sizeof(struct udp_hdr), msg, strlen(msg));

					/* Send it.. */
					memcpy(socket_address.sll_addr, dst_mac, 6);
					if (sendto(sockfd, buffer_u.raw_data, sizeof(struct eth_hdr) + sizeof(struct ip_hdr) + sizeof(struct udp_hdr) + strlen(msg), 0, (struct sockaddr*)&socket_address, sizeof(struct sockaddr_ll)) < 0)
						printf("Send failed\n");

					//printf("send\n");

								//fim echo
				}
						continue;
			}
		}		
				//printf("got a packet, %d bytes\n", numbytes);
	}

	return 0;
}

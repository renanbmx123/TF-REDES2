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

int iphdrlen;
FILE *pFile;
char ifName[100];
unsigned char * data;

struct sockaddr saddr;
struct sockaddr_in source,dest;
struct ifreq ifreq_c,ifreq_i,ifreq_ip; /// for each ioctl keep diffrent ifreq structure otherwise error may come in sending(sendto )

struct iphdr *ip;
struct ethhdr *eth;
struct udphdr *udp;

// Funçoes Auxiliares
void get_eth_index(int sock_raw)
{
	memset(&ifreq_i,0,sizeof(ifreq_i));
	strncpy(ifreq_i.ifr_name,ifName,IFNAMSIZ-1);

	if((ioctl(sock_raw,SIOCGIFINDEX,&ifreq_i))<0)
		printf("error in index ioctl reading");

	printf("index=%d\n",ifreq_i.ifr_ifindex);

}

void get_mac(int sock_raw)
{
	memset(&ifreq_c,0,sizeof(ifreq_c));
	strncpy(ifreq_c.ifr_name,ifName,IFNAMSIZ-1);

	if((ioctl(sock_raw,SIOCGIFHWADDR,&ifreq_c))<0)
		printf("error in SIOCGIFHWADDR ioctl reading");
	printf("Mac= %.2X-%.2X-%.2X-%.2X-%.2X-%.2X\n",(unsigned char)(ifreq_c.ifr_hwaddr.sa_data[0]),(unsigned char)(ifreq_c.ifr_hwaddr.sa_data[1]),(unsigned char)(ifreq_c.ifr_hwaddr.sa_data[2]),(unsigned char)(ifreq_c.ifr_hwaddr.sa_data[3]),(unsigned char)(ifreq_c.ifr_hwaddr.sa_data[4]),(unsigned char)(ifreq_c.ifr_hwaddr.sa_data[5]));
}

void ethernet_header(unsigned char* buffer,int buflen)
{
	  eth = (struct ethhdr *)(buffer);
}

void ip_header(unsigned char* buffer,int buflen)
{
	ip = (struct iphdr*)(buffer + sizeof(struct ethhdr));

	iphdrlen =ip->ihl*4;

	memset(&source, 0, sizeof(source));
	source.sin_addr.s_addr = ip->saddr;
	memset(&dest, 0, sizeof(dest));
	dest.sin_addr.s_addr = ip->daddr;
  // fprintf(pFile , "\nIP Header\n");
}

void payload(unsigned char* buffer,int buflen)
{

}

void udp_header(unsigned char* buffer, int buflen)
{
	ethernet_header(buffer,buflen);
	ip_header(buffer,buflen);

	udp = (struct udphdr*)(buffer + iphdrlen + sizeof(struct ethhdr));
  // if(!memcmp(buffer,src, sizeof(src))){
  //    // IP
     // printf("\nIP Header\n");
	   // printf("\t|-Version              : %d\n",(unsigned int)ip->version);
	   // printf("\t|-Internet Header Length  : %d DWORDS or %d Bytes\n",(unsigned int)ip->ihl,((unsigned int)(ip->ihl))*4);
     // printf("\t|-Type Of Service   : %d\n",(unsigned int)ip->tos);
	   // printf("\t|-Total Length      : %d  Bytes\n",ntohs(ip->tot_len));
     // printf("\t|-Identification    : %d\n",ntohs(ip->id));
	   // printf("\t|-Time To Live	    : %d\n",(unsigned int)ip->ttl);
	   // printf("\t|-Protocol 	    : %d\n",(unsigned int)ip->protocol);
	   // printf("\t|-Header Checksum   : %d\n",ntohs(ip->check));
	   // printf("\t|-Source IP         : %s\n", inet_ntoa(source.sin_addr));
	   // printf("\t|-Destination IP    : %s\n",inet_ntoa(dest.sin_addr));
     // // UDP
     // printf("\t|-Source Port    	: %d\n" , ntohs(udp->source));
   	 // printf("\t|-Destination Port	: %d\n" , ntohs(udp->dest));
   	 // printf("\t|-UDP Length      	: %d\n" , ntohs(udp->len));
   	 // printf( "\t|-UDP Checksum   	: %d\n" , ntohs(udp->check));
  // }
	payload(buffer,buflen);

}

void data_process(unsigned char* buffer,int buflen)
{
	struct iphdr *ip = (struct iphdr*)(buffer + sizeof (struct ethhdr));
  udp_header(buffer,buflen);
  if(ip->protocol == 17){
    if(!memcmp(inet_ntoa(dest.sin_addr),"192.168.0.188",sizeof(dest.sin_addr))) {
      if((ntohs(udp->dest) == 5002)){

        // Grab file parts
        int i = 0;
        unsigned char * data = (buffer + iphdrlen  + sizeof(struct ethhdr) + sizeof(struct udphdr));
        int remaining_data = buflen - (iphdrlen  + sizeof(struct ethhdr) + sizeof(struct udphdr));
        for(i=0;i<remaining_data;i++)
        {
          fprintf(pFile,"%c",data[i]);
        }

      }

	   }
}
}

int main(int argc, char *argv[])
{

	int sock_r,saddr_len,buflen;

	unsigned char* buffer = (unsigned char *)malloc(65536);
	memset(buffer,0,65536);

  if (argc > 1)
      strcpy(ifName, argv[1]);
    else
    strcpy(ifName, "eth0");

	pFile=fopen("RECEBIDO.txt","w");
	if(!pFile)
	{
		printf("unable to open log.txt\n");
		return -1;

	}

	printf("starting .... \n");

	sock_r=socket(AF_PACKET,SOCK_RAW,htons(ETH_P_ALL));
	if(sock_r<0)
	{
		printf("error in socket\n");
		return -1;
	}

	while(1)
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

	printf("DONE!!!!\n");

}

// int main(int argc, char *argv[]) {
//   int saddr_len,buflen;
//   struct ifreq ifopts, ifreq_c;	/* set promiscuous mode */
// 	struct ifreq if_ip;	/* get ip addr */
//
//   int packet_size;
//
//   if (argc > 1)
//     strcpy(ifName, argv[1]);
//   else
//   strcpy(ifName, "eth0");
//
//   pFile=fopen ("RECEBIDO.txt","w");
//   //Allocate string buffer to hold incoming packet data
//   unsigned char* buffer = (unsigned char *)malloc(65536);
//   memset(buffer,0,65536);
//
//   // Open the raw socket
//   int sock_r = socket (PF_INET, SOCK_RAW, IPPROTO_TCP);
//   if(sock_r == -1){
//     //socket creation failed, may be because of non-root
//     perror("Failed to create socket");
//     exit(1);
//   }
//
//   get_eth_index(sock_r); //Get interface index hw
//   get_mac(sock_r); // Get mac address from interface
//
//   /* Set interface to promiscuous mode - do we need to do this every time? */
// 	strncpy(ifopts.ifr_name, ifName, IFNAMSIZ-1);
// 	ioctl(sock_r, SIOCGIFFLAGS, &ifopts);
// 	ifopts.ifr_flags |= IFF_PROMISC;
// 	ioctl(sock_r, SIOCSIFFLAGS, &ifopts);
//
//   while (1) {
//       saddr_len=sizeof saddr;
//
//   		buflen=recvfrom(sock_r,buffer,65536,0,&saddr,(socklen_t *)&saddr_len);
//     	if(buflen<0)
//   		{
//   			printf("error in reading recvfrom function\n");
//   			return -1;
//   		}
//       //printf(" %d\n", buflen );
//       data_process(buffer,buflen);
//       memset(buffer,0,sizeof(buffer));
//     }
//     return 0;
//   }

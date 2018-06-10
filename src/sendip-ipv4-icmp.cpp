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

int main(int argc, char **argv) {

	printf("######################################\n");
	printf(
			"#\tsendip-ipv4-icmp             #\n#\tWersja v1.0                  #\n#\tWykonał Patryk Sikora        #\n");
	printf("######################################\n");

	if (argc < 3) {
		printf(
				"\nUżycie: %s <źródłowy IP> <docelowy IP> [-a], spróbuj ponownie.\n",
				argv[0]);
		exit(0);
	}

	unsigned long daddr = inet_addr(argv[2]);
	unsigned long saddr = inet_addr(argv[1]);

	int payload_size = 0, sent, sent_size, sent_total = 1000000;

	int sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);

	if (sockfd < 0) {
		perror("błąd otwarcia socketu");
		return (0);
	}

	int on = 1;

	// generowanie naglowka IP
	if (setsockopt(sockfd, IPPROTO_IP, IP_HDRINCL, (const char*) &on,
			sizeof(on)) == -1) {
		perror("błąd generowania nagłówka ip");
		return (0);
	}

	//wysylanie do adresow broadcast
	if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, (const char*) &on,
			sizeof(on)) == -1) {
		perror("błąd uprawnień");
		return (0);
	}

	//calkowita wielkosc pakietu
	int packet_size = sizeof(struct iphdr) + sizeof(struct icmphdr)
			+ payload_size;
	char *packet = (char *) malloc(packet_size);

	if (!packet) {
		perror("błąd alokowania pamięci");
		close(sockfd);
		return (0);
	}

	//struktura naglowka ip
	struct iphdr *ip = (struct iphdr *) packet;
	struct icmphdr *icmp = (struct icmphdr *) (packet + sizeof(struct iphdr));

	//wypełnia pamięć bajtem
	memset(packet, 0, packet_size);

	if (argc < 4) {
		ip->tos = 0;
		ip->frag_off = 0;
		ip->ttl = 255;
		icmp->type = ICMP_ECHO;
		icmp->code = 0;
		icmp->un.echo.id = rand();
	} else if (strcmp(argv[3], "-a") == 0) {
		printf("\nPodaj ilość pakietów do wysłania: ");
		scanf("%d", &sent_total);
		printf("Podaj TOS: ");
		scanf("%d", &ip->tos);
		printf("Fragment offset: ");
		scanf("%d", &ip->frag_off);
		printf("Podaj TTL (0-255): ");
		scanf("%d", &ip->ttl);
		printf("Podaj typ ICMP: ");
		scanf("%d", &icmp->type);
		printf("Podaj kod ICMP: ");
		scanf("%d", &icmp->code);
		printf("Podaj id pakietu ICMP: ");
		scanf("%d", &icmp->un.echo.id);
	}

	ip->version = 4;
	ip->id = rand();
	ip->protocol = IPPROTO_ICMP;
	ip->saddr = saddr;
	ip->daddr = daddr;
	ip->tot_len = htons(packet_size);
	ip->ihl = 5;
	ip->check = chsum((u16 *) ip, sizeof(struct iphdr));
	icmp->un.echo.sequence = rand();
	icmp->checksum = 0;

	struct sockaddr_in servaddr;
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = 1234;
	servaddr.sin_addr.s_addr = daddr;
	memset(&servaddr.sin_zero, 0, sizeof(servaddr.sin_zero));

	printf("\n\tWYSYŁANIE...\n");

	while (sent < sent_total) {
		memset(packet + sizeof(struct iphdr) + sizeof(struct icmphdr),
				rand() % 255, payload_size);

		//przeliczenia sumy kontorlnej nag. icmp przy uzupenianiu payload randem
		icmp->checksum = 0;
		icmp->checksum = chsum((unsigned short *) icmp,
				sizeof(struct icmphdr) + payload_size);

		if ((sent_size = sendto(sockfd, packet, packet_size, 0,
				(struct sockaddr*) &servaddr, sizeof(servaddr))) < 1) {
			perror("błąd wysyłania\n");
			break;
		}
		++sent;
		printf("\t%d pakietów wysłanych\r", sent);
		fflush(stdout);

		usleep(100000);  //mikrosekundy
	}

	printf("\tWYSŁANO %d PAKIET(Y/ÓW)\n\n", sent_total);

	free(packet);
	close(sockfd);

	return (0);
}

//Funkcja liczenia sumy kontrolnej
unsigned short chsum(unsigned short *ptr, int nbytes) {
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
		*((u_char *) &oddbyte) = *(u_char *) ptr;
		sum += oddbyte;
	}

	sum = (sum >> 16) + (sum & 0xffff);
	sum += (sum >> 16);
	answer = ~sum;

	return (answer);
}

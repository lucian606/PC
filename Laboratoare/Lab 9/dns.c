// Protocoale de comunicatii
// Laborator 9 - DNS
// dns.c

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>

int usage(char* name)
{
	printf("Usage:\n\t%s -n <NAME>\n\t%s -a <IP>\n", name, name);
	return 1;
}

// Receives a name and prints IP addresses
void get_ip(char* name)
{
	int ret;
	struct addrinfo hints, *result, *p;

	// TODO: set hints
	memset(&hints, 0, sizeof(hints));
	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = AF_UNSPEC;
	// TODO: get addresses
	ret = getaddrinfo(name, NULL, &hints, &result);
	if (ret < 0)
		perror("Invalid ret");
	p = result;
	while(p != NULL) {
		if(p->ai_family == AF_INET) {
			char ip[INET_ADDRSTRLEN];
			struct sockaddr_in *addr = (struct sockaddr_in*) p->ai_addr;
			if(inet_ntop(p->ai_family, &(addr->sin_addr), ip, sizeof(ip)) != NULL)
				printf("IP: %s  PORT: %d\n", ip, ntohs(addr->sin_port));
			if (p->ai_canonname != NULL)
				printf("%s %d\n", p->ai_canonname, p->ai_socktype);
		}
		else if (p->ai_family == AF_INET6) {
			char ip[INET6_ADDRSTRLEN];
			struct sockaddr_in6 *addr = (struct sockaddr_in6*) p->ai_addr;
			if(inet_ntop(p->ai_family, &(addr->sin6_addr), ip, sizeof(ip)) != NULL)
				printf("IP: %s Port: %d\n", ip, ntohs(addr->sin6_port));
			if (p->ai_canonname != NULL)
				printf("%s %d\n", p->ai_canonname, p->ai_socktype);
		}
		p = p->ai_next;
	}

	// TODO: iterate through addresses and print them
	free(result);
	// TODO: free allocated data
}

// Receives an address and prints the associated name and service
void get_name(char* ip)
{
	int ret;
	struct sockaddr_in addr;
	char host[1024];
	char service[20];

	// TODO: fill in address data
	addr.sin_family = AF_INET;
	addr.sin_port = htons(8080);
	inet_aton(ip, &addr.sin_addr);

	// TODO: get name and service
	getnameinfo ((struct sockaddr*)&addr, sizeof(struct sockaddr_in), host, 1024, service, 20, 0);

	printf("Name: %s Service: %s\n", host, service);

	// TODO: print name and service
}

int main(int argc, char **argv)
{
	if (argc < 3) {
		return usage(argv[0]);
	}

	if (strncmp(argv[1], "-n", 2) == 0) {
		get_ip(argv[2]);
	} else if (strncmp(argv[1], "-a", 2) == 0) {
		get_name(argv[2]);
	} else {
		return usage(argv[0]);
	}

	return 0;
}

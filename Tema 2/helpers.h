#ifndef _HELPERS_H
#define _HELPERS_H 1

#include <stdio.h>
#include <stdlib.h>

#define SUBSCRIBE 1
#define UNSUBSCRIBE 2
#define BUFLEN 1584
#define CLIENT_BUFLEN 1584
#define MAX_CLIENTS	5000	// numarul maxim de clienti in asteptare
#define SUCCESS 1
#define FAIL 2

/*
 * Macro de verificare a erorilor
 * Exemplu:
 *     int fd = open(file_name, O_RDONLY);
 *     DIE(fd == -1, "open failed");
 */

#define DIE(assertion, call_description)	\
	do {									\
		if (assertion) {					\
			fprintf(stderr, "(%s, %d): ",	\
					__FILE__, __LINE__);	\
			perror(call_description);		\
			exit(EXIT_FAILURE);				\
		}									\
	} while(0)

typedef struct __attribute__((__packed__)) {
	char topic[50];
	unsigned char data_type;
	char content[1501];
} udp_msg; // Structura unui mesaj udp

typedef struct __attribute__((__packed__)) {
	char request_type;
	char sf_type;
	char topic[50];
} request_msg; // Structura unei cereri de subscribe/unsubscribe

typedef struct __attribute__((__packed__)) {
	char topic[50];
	unsigned char data_type;
	char content[1501];
	u_short port;
	char ip[30];
} tcp_msg; // Structura unui mesaj trimis catre un client tcp

char data_types[4][15] = {
	"INT", "SHORT_REAL", "FLOAT", "STRING"
}; // Vectora care contine tipurile de date

typedef struct __attribute__((__packed__)) {
	unsigned char reply_type;
} sub_reply; // Raspunsul unei cereri de subscribe/unsubscribe

#endif

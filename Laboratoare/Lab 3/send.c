#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "lib.h"

#define HOST "127.0.0.1"
#define PORT 10000

int main(int argc, char *argv[])
{
	msg t;
	pkt p;
	int i, res;
	int j, k;
	int BDP=atoi(argv[1]);	//BDP=SPEED*DELAY
	int window_size=(BDP*1000)/(8*sizeof(msg));	//Calculam dimensiunea ferestrei
	printf("[SENDER] Starting.\n");	
	init(HOST, PORT);

	/* printf("[SENDER]: BDP=%d\n", atoi(argv[1])); */
	//COUNT=NR DE PACHETE CARE SUNT TRIMISE IN TOTAL
	for(i = 0; i < window_size; i++) {
			memset(&t, 0, sizeof(msg));
			memset(&p, 0, sizeof(pkt));
			sprintf(p.payload, "Ana are %d mere!\n", i);
			/* gonna send an empty msg */
			t.len = strlen(p.payload)+1+sizeof(int);
			p.type=1;	//MESSAGE TYPE
			memcpy(t.payload, &p, sizeof(pkt));
			/* send msg */
			t.parity='0';
			for(k=0; k < t.len; k++) {
				for(j=0; j < 8; j++) {
					t.parity^=(1 << j)&t.payload[k];
				}
			}
			printf("Send par: %d \n", t.parity);
			res = send_message(&t);
			/*if (res < 0) {
				perror("[SENDER] Send error. Exiting.\n");
				return -1;
			}
			else {			
				printf("Message sent successfully.\n");
			}	*/
		}	

	for(i=0; i < COUNT-window_size; i++) {
		memset(&t, 0, sizeof(msg));
		memset(&p, 0, sizeof(pkt));
		res=recv_message(&t);
		if(res < 0) {
			perror("ACK not received.\n");
			return -1;
		}
		memcpy(&p, t.payload, sizeof(pkt));
		if(p.type == 0) {
			printf("%s\n", p.payload);
			//memset(&t, 0, sizeof(msg));
			//memset(&p, 0, sizeof(pkt));
			sprintf(p.payload, "Ana are %d mere!\n", i);
			/* gonna send an empty msg */
			t.len = strlen(p.payload)+1+sizeof(int);
			p.type=1;	//MESSAGE TYPE
			memcpy(t.payload, &p, sizeof(pkt));
			/* send msg */
			t.parity='0';
			for(k=0; k < t.len; k++) {
				for(j=0; j < 8; j++) {
					t.parity^=(1 << j)&t.payload[k];
				}
			}
			printf("SEND PAR: %d\n", t.parity);
			res = send_message(&t);
			if (res < 0) {
				perror("[SENDER] Send error. Exiting.\n");
				return -1;
			}
			else {
				printf("Message sent successfully.\n");
				}
			}
	}

	for(i=0; i < window_size; i++) {
		memset(&t, 0, sizeof(msg));
		memset(&p, 0, sizeof(pkt));
		res=recv_message(&t);
			if(res < 0) {
			perror("ACK not received.\n");
			return -1;
		}
		memcpy(&p, t.payload, sizeof(pkt));
		if(p.type == 0) {
			printf("%s\n", p.payload);
		}
	}

	printf("[SENDER] Job done, all sent.\n");
		
	return 0;
}

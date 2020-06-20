#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include "lib.h"

#define HOST "127.0.0.1"
#define PORT 10001

int main(void)
{
	msg r;
	pkt p;
	int i,j ,k , res;
	
	printf("[RECEIVER] Starting.\n");
	init(HOST, PORT);
	char checkSum;
	for (i = 0; i < COUNT; i++) {
		/* wait for message */
		//memset(&r, 0, sizeof(msg));
		checkSum='0';
		//printf("R: %d\n", r.parity);
		res = recv_message(&r);
			printf("R: %d\n", r.len);
		if (res < 0) {
			perror("[RECEIVER] Receive error. Exiting.\n");
			return -1;
		}
		for(k=0; k < r.len; ++k) {
				for(j=0; j < 8; ++j) {
					checkSum^=(1 << j)&r.payload[k];
				}
		}
		printf("SUM: %d---- PAR: %d\n", checkSum, r.parity);
		if(checkSum != r.parity) {
			memset(&p, 0, sizeof(pkt));
			memcpy(&p, r.payload, sizeof(pkt));
			if(p.type == 1) {
			printf("Message received with success: %s\n", p.payload);
		/* send dummy ACK */
			//memset(&r, 0, sizeof(msg));
			memset(&p, 0, sizeof(pkt));
			sprintf(p.payload, "NACK\n");
			/* gonna send an empty msg */
			r.len = strlen(p.payload)+1+sizeof(int);
			p.type=0;	//ACK TYPE
			memcpy(r.payload, &p, sizeof(pkt));
			/* send msg */
			res = send_message(&r);
			if (res < 0) {
				perror("[RECEIVER] Send ACK error. Exiting.\n");
				return -1;
				}
			}
		}
		else {
		memset(&p, 0, sizeof(pkt));
		memcpy(&p, r.payload, sizeof(pkt));
		if(p.type == 1) {
			printf("Message received with success: %s\n", p.payload);
		/* send dummy ACK */
			memset(&r, 0, sizeof(msg));
			memset(&p, 0, sizeof(pkt));
			sprintf(p.payload, "ACK\n");
			/* gonna send an empty msg */
			r.len = strlen(p.payload)+1+sizeof(int);
			p.type=0;	//ACK TYPE
			memcpy(r.payload, &p, sizeof(pkt));
			/* send msg */
			res = send_message(&r);
			if (res < 0) {
				perror("[RECEIVER] Send ACK error. Exiting.\n");
				return -1;
				}
			}
		}
	}

	printf("[RECEIVER] Finished receiving..\n");
	return 0;
}

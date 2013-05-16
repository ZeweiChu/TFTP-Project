//tftp.c
#include "tftp.h"

recvData(int sockfd, struct sockaddr *pserv_addr, int servlen, char *filename){
	int n, clilen;
	
	short signed block = 0;
	
	struct TFTP_DATA *dataPacket = malloc(sizeof(struct TFTP_DATA));
	struct TFTP_ACK *ack = malloc(sizeof(struct TFTP_ACK));
	FILE *fp = fopen(filename, "w");
	char buffer[MAX];

	for (;;){
		clilen = sizeof(struct sockaddr);

		n = recvfrom(sockfd, dataPacket, sizeof(struct TFTP_DATA), 0, pserv_addr, &servlen);
		if (n < 0) {
			printf(stderr, "Error on receiving DATA\n");
			//TODO: 
		}

		if (ntohs(dataPacket->opcode) == ERROR) {
			printf(stderr, "Error received - %s\n", errMsg[ntohs(dataPacket->block)]);
			//TODO: 
		}

		int length = strlen(dataPacket->data);
		(buffer, dataPacket->data);
		
	}
}

sendData(int sockfd, struct sockaddr *recv_addr, int recvlen, char *filename){
	int n, len;

	signed short int block = 0;
	FILE *fp = fopen(filename, "r");
	char buffer[MAX];

	if (fp == NULL){
		//TODO: print out error mesg and send error mesg
	}

	struct TFTP_DATA * dataPacket;
	struct TFTP_ACK * ack;
	dataPacket = (struct TFTP_DATA *)malloc(sizeof(struct TFTP_DATA));
	memset(dataPacket, 0, sizeof(struct TFTP_DATA));
	ack = (struct TFTP_ACK*)malloc(sizeof(struct TFTP_ACK));
	memset(ack, 0, sizeof(struct TFTP_ACK));


}

processError()


int main()

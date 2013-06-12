#include "tftp.c"

char* usage = "usage: tftpserver [-p] [port]";

char* progname;


loop(sockfd)
int sockfd;
{
	struct sockaddr pcli_addr;
	int n, clilen;
	char mesg[MAX_SIZE];
	printf("entering mainloop: \n");
	for (;;){
		clilen = sizeof(struct sockaddr);
		
		bzero(mesg, MAX_SIZE);
		n = recvfrom(sockfd, mesg, MAX_SIZE, 0, &pcli_addr, &clilen);
	
		if (n < 0)
		{
			printf("%s: recvfrom error on request\n", progname);
			exit(3);	
		}
		unsigned short* p = (unsigned short*) mesg;
		unsigned short opcode = ntohs(*p);
		char* filename = mesg+2;
		char* mode = mesg + 2 + strlen(filename) + 1;
		
		if (fork() == 0){
			close(sockfd);
			int child_sockfd;
			struct sockaddr_in serv_addr;
			if ((child_sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
				printf("cannot open datagram in child\n");
				exit(1);
			}
			bzero((char*)&serv_addr, sizeof(serv_addr));
			serv_addr.sin_family = AF_INET;
			serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
			serv_addr.sin_port = htons(0);
			if (bind(child_sockfd, (struct sockaddr* )&serv_addr, sizeof(serv_addr)) < 0){
				printf("cannot bind local address in child\n");
				exit(2);
			}
			if (strcmp(mode, "octet") != 0){
				printf("Server cannot identify the request mode\n");
				exit(2);
			}
			if (opcode == RRQ){
				printf("RRQ received\n");
				printf("start sending file %s...\n", filename);
				sending(child_sockfd, pcli_addr, clilen, filename);
			} else if (opcode == WRQ){
				printf("WRQ received\n");
				printf("start receiving file %s...\n", filename);
				receiving(child_sockfd, pcli_addr, clilen, filename);
			}
			exit(1);
		}
	}
}

sending(sockfd, pcli_addr, clilen, filename)
int sockfd;
struct sockaddr pcli_addr;
int clilen;
char* filename;
{
	init();
	int curr_block = 1;
	int block = 0;
	int status = DATA;
	char data_pkt[MAX_SIZE];
	char mesg[MAX_SIZE];
	char data[MAX_DATA];
	unsigned short* p;
	char* c;
	int n;
		

	FILE* fp = fopen(filename, "r");
	if (fp == NULL){
		bzero(mesg, MAX_SIZE);
		p = (unsigned short*)mesg;
		*p = htons(ERROR);
		p++;
		*p = htons(1);
		n = 4;
		printf("sending error block...\n");
		if (sendto(sockfd, mesg, n, 0, &pcli_addr, clilen) != n){
			printf("Error on sending error packet\n");
			exit(4);
		}
		processError(1);	
	}
	if (access(filename, R_OK) == -1){
		bzero(mesg, MAX_SIZE);
		p = (unsigned short*)mesg;
		*p = htons(ERROR);
		p++;
		*p = htons(2);
		n = 4;
		printf("sending error block\n");
		if (sendto(sockfd, mesg, n, 0, &pcli_addr, clilen) != n){
			printf("Error on sending error packet\n");
			exit(4);
		}
		processError(2);
	}

	
	while (status == DATA){
		bzero(data_pkt, MAX_SIZE);
		bzero(data, MAX_DATA);
		p = (unsigned short*)data_pkt;
		*p = htons(DATA);
		p = (unsigned short*)(data_pkt+2);
		*p = htons(curr_block);
		c = data_pkt+4;
		n = fread(data, sizeof(char), MAX_DATA, fp);
		bcopy(data, c, n);
		n+=4;
		status = SENDING;
		while (status == SENDING){
			printf("sending data block #%d...\n", curr_block);
			if (sendto(sockfd, data_pkt, n, 0, &pcli_addr, clilen) != n){
				printf("sendto error on block %d\n", curr_block);
				exit(4);
			}
			while (1){
				alarm(1);
				bzero(mesg, MAX_SIZE);
				recv_len = recvfrom(sockfd, mesg, MAX_SIZE, 0, &pcli_addr, &clilen);
				if (recv_len < 0){
					if (timeout >= 10){
						printf("connection lost\n");
						exit(3);
					}
				} else {
					alarm(0);
					timeout = 0;
					status = DATA;
					if (n < MAX_DATA) status = ENDING;
					p = (unsigned short*)mesg;
					p++;
					block = ntohs(*p);
					if (block == curr_block){
						curr_block++;
						break;
					}
					printf("received ACK #%d\n", block);
				}
			}
		}
	}
	fclose(fp);

}

receiving(sockfd, pcli_addr, clilen, filename)
int sockfd;
struct sockaddr pcli_addr;
int clilen;
char* filename;
{
	init();
	char mesg[MAX_SIZE];
	char data[MAX_DATA];
	char ack_pkt[MAX_SIZE];
	int status = DATA;
	int curr_block = 0;
	int block = 0;
	unsigned short* p;
	int n;
	if (access(filename, 0) == 0){
		bzero(ack_pkt, MAX_SIZE);
		p = (unsigned short*)ack_pkt;
		*p = htons(ERROR);
		p++;
		*p = htons(6);
		n = 4;
		printf("sending error block\n");
		if (sendto(sockfd, ack_pkt, n, 0, &pcli_addr, clilen) != n){
			printf("Error on sending error packet\n");
			exit(4);
		}
		processError(6);
	}
	
	FILE* fp = fopen(filename, "w");
	if (access(filename, W_OK) == -1){
		bzero(ack_pkt, MAX_SIZE);
		p = (unsigned short*)ack_pkt;
		*p = htons(ERROR);
		p++;
		*p = htons(2);
		n = 4;
		printf("sending error block\n");
		if (sendto(sockfd, ack_pkt, n, 0, &pcli_addr, clilen) != n){
			printf("Error on sending error packet\n");
			exit(4);
		}
		processError(2);
		
	}


	while (status == DATA){

		bzero(ack_pkt, MAX_SIZE);
		p = (unsigned short*)ack_pkt;
		*p = htons(ACK);
		p++;
		*p = htons(curr_block);
		n = 4;
		while (1){
			alarm(1);
			printf("sending ACK block #%d\n", curr_block);
			if (sendto(sockfd, ack_pkt, n, 0, &pcli_addr, clilen) != n){
				printf("sendto error on ACK #0\n");
				printf("%s\n", strerror(errno));
				exit(4);
			}
			bzero(mesg, MAX_SIZE);
			recv_len = recvfrom(sockfd, mesg, MAX_SIZE, 0, NULL, NULL);
			if (recv_len < 0){
				if (timeout >= 10){
					printf("connection lost\n");
					exit(3);
				}
			} else {
				alarm(0);
				timeout = 0;
				p = (unsigned short*)(mesg+2);
				block = ntohs(*p);
				printf("received data block #%d\n", block);
				bzero(data, MAX_DATA);
				n = recv_len - 4;
				if (n < MAX_DATA){
					status = ENDING;
					printf("last block\n");
				}
				bcopy(mesg+4, data, n);
				if (block > curr_block) {
					curr_block = block;
					fwrite(data, sizeof(char), n, fp);
					break;
				}
			}
		}
	}

	bzero(ack_pkt, MAX_SIZE);
	p = (unsigned short*)ack_pkt;
	*p = htons(ACK);
	p++;
	*p = htons(curr_block);
	n = 4;
	printf("sending ACK block #%d...\n", curr_block);
	if (sendto(sockfd, ack_pkt, n, 0, &pcli_addr, clilen) != n)
	{
		printf("sending ACK block #%d error\n", curr_block);
		exit(4);
	}
		
	
	fclose(fp);
}


main(argc, argv)
int argc;
char *argv[];
{
	printf(title);
	int sockfd;
	struct sockaddr_in serv_addr;
	progname = argv[0];
	int port = SERV_UDP_PORT;
	if (argc != 1 && argc != 3){
		printf("wrong number of arguments\n");
		printf(usage);
		exit(1);
	}	
	if (argc == 3){
		if (strcmp(argv[1], "-p") == 0){
			port = atoi(argv[2]);
			printf("port number changed to %s\n", argv[2]);
		} else {
			printf("wrong arguments\n");
			printf(usage);
			exit(1);
		}
	}

	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		printf("%s: can't open datagram socket\n", progname);
		exit(1);
	}

	bzero((char*)&serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(port);
	
	if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0){
		printf("%s: can't bind local address, %s\n", progname, strerror(errno));
		exit(2);
	}

	loop(sockfd);
}

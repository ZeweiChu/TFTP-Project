#include "tftp.c"

char* usage = "usage: tftpclient -r/-w filename [-p] [port]\n";

char* progname;

dg_cli(sockfd, pserv_addr, servlen, filename, mode)
int sockfd;
struct sockaddr* pserv_addr;
int servlen;
char* filename;
int mode;
{
	struct sockaddr child_pserv_addr;
	int child_servlen;
	char rq[MAX_SIZE];
	bzero(rq, MAX_SIZE);
	unsigned short* p = (unsigned short*)rq;
	*p = htons(mode);
	char* c = rq+2;
	strcpy(c, filename);
	c += (strlen(filename) + 1);
	strcpy(c, "octet"); 
	int n = 2 + strlen(filename) + 7;


	int status;
	if (mode == RRQ){
		
		status = RRQ;
		unsigned short curr_block = 0;
		unsigned short block = 0;
		FILE* fp;
		if (access(filename, 0) == 0){
			printf("File already exists\n");
			exit(1);
		}
		char mesg[MAX_SIZE];
		char data[MAX_DATA];
		char ack_pkt[MAX_DATA];
		unsigned short* p;
		char* c;
		int opcode;
		while (status == RRQ){
			while (1){
				alarm(1);
				printf("sending rrq of file %s...\n", filename);
				if (sendto(sockfd, rq, n, 0, pserv_addr, servlen) != n){
					printf("sending request error\n");
					exit(3);
				}
				bzero(mesg, MAX_SIZE);
				recv_len = recvfrom(sockfd, mesg, MAX_SIZE, 0, &child_pserv_addr, &child_servlen);
				curr_block = block;
				if (recv_len < 0){
						if (timeout >= 10){
						printf("connection lost.\n");
						exit(4);
					}
				} else {
					alarm(0);
					timeout = 0;
					fp = fopen(filename, "w");
					unsigned short* p = (unsigned short*)mesg;
					opcode = ntohs(*p);
					if (opcode == ERROR) processError(mesg);
					p++;
					block = ntohs(*p);
					printf("received data block #%d\n", block);
					if (block > curr_block) {
						status = DATA;
						curr_block = block;
						break; 
					}
					n = recv_len - 4;
					if (n < MAX_DATA) status = ENDING;
					bzero(data, MAX_DATA);
					bcopy(mesg+4, data, n);
					fwrite(data, sizeof(char), n, fp);
				}
			}
		}
		while (status == DATA){
			bzero(ack_pkt, MAX_SIZE);
			p = (unsigned short*)ack_pkt;
			*p = htons(ACK);
			p++;
			*p = htons(curr_block);
			p++;
			n = 4;
			while (1){
				alarm(1);
				printf("sending ack block #%d\n", curr_block);
				if (sendto(sockfd, ack_pkt, n, 0, &child_pserv_addr, child_servlen) != n){
					printf("sending ack block #%d error on socket\n", curr_block);
					exit(3);
				}
				bzero(mesg, MAX_SIZE);
				recv_len = recvfrom(sockfd, mesg, MAX_SIZE, 0, &child_pserv_addr, &child_servlen);
				if (recv_len < 0){
					if (timeout >= 10){	
						printf("connection lost\n");
						exit(4);
					}
				} else {
					alarm(0);
					timeout = 0;
					p = (unsigned short*)(mesg+2);
					block = ntohs(*p);
					c = mesg + 4; 
				
					printf("received block #%d of data\n", block);
					bzero(data, MAX_DATA);
					n = recv_len - 4;
					if (n < MAX_DATA) status = ENDING;
					bcopy(c, data, n);
					if (block > curr_block){
						fwrite(data, sizeof(char), n, fp);
						curr_block = block;
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
		printf("sending ack block #%d\n", curr_block);
		if (sendto(sockfd, ack_pkt, n, 0, &child_pserv_addr, child_servlen) != n){
			printf("sending ack block #%d error on socket\n", curr_block);
			exit(3);
		}
		

		fclose(fp);
			
	} else {
		status = WRQ;
		unsigned short block = 0;
		if (access(filename, R_OK) == -1){
			printf("no permission to access the file\n");
			exit(1);
		}
		FILE* fp = fopen(filename, "r");
		if (fp == NULL) {
			printf("file not found\n");
			exit(1);
		}
		char mesg[MAX_SIZE];
		unsigned short curr_block = 0;
		char data[MAX_DATA];
		char data_pkt[MAX_SIZE];
		unsigned short* p;
		char* c;
		while (status == WRQ){
			alarm(1);
			printf("sending wrq of file %s...\n", filename);
			if (sendto(sockfd, rq, n, 0, pserv_addr, servlen) != n){
				printf("sending request error\n");
				exit(3);
			}	
			while (1){
				bzero(mesg, MAX_SIZE);
				recv_len = recvfrom(sockfd, mesg, MAX_SIZE, 0, &child_pserv_addr, &child_servlen);
				if (recv_len < 0){
					if (timeout >= 10){
						printf("connection lost.\n");
						exit(4);
					}
				} else {
					status = DATA;
					alarm(0);
					timeout = 0;
					p = (unsigned short*)mesg;
					int opcode = ntohs(*p);
					if (opcode == ERROR) processError(mesg);
					p++;
					block = ntohs(*p);
					printf("received ACK block #%d\n", block);
					printf("%s\n", &child_pserv_addr);
					if (block == curr_block){
						curr_block++;
						break;
					}

				}
			}
		}
		while (status == DATA){
			bzero(data, MAX_DATA);
			n = fread(data, sizeof(char), MAX_DATA, fp);			
			bzero(data_pkt, MAX_SIZE);
			p = (unsigned short*)data_pkt;
			*p = htons(DATA);
			p++;
			*p = htons(curr_block);
			p++;
			c = data_pkt + 4;
			bcopy(data, c, n);
			n+=4;
			status = SENDING;
			while (status == SENDING){
				alarm(1);
				printf("sending data block #%d...\n", curr_block);
				printf("%s\n", child_pserv_addr);
				if (sendto(sockfd, data_pkt, n, 0, &child_pserv_addr, child_servlen) != n){
					printf("error on sending data block #%d...\n", curr_block);
					printf("%s\n", strerror(errno));
					exit(3);
				}
				while (1){
					bzero(mesg, MAX_SIZE);
					recv_len = recvfrom(sockfd, mesg, MAX_SIZE, 0, &child_pserv_addr, &child_servlen);
					if (recv_len < 0){
						if (timeout >= 10){	
							printf("connection lost\n");
							exit(4);
						}
					} else {
						status = DATA;
						if (n < MAX_SIZE) status = ENDING;
						alarm(0);	
						timeout = 0;
						p = (unsigned short*)(mesg+2);
						int block = ntohs(*p);
						printf("received ACK block #%d\n", block);
						if (block == curr_block){
							curr_block++;
							break;
						}
					}
				}
			}
		}	
		fclose(fp);
	}
}

main(argc, argv)
int 	argc;
char 	*argv[];
{
	int sockfd;
	struct sockaddr_in cli_addr, serv_addr;
	int port = SERV_UDP_PORT;
	progname = argv[0];
	int mode;
	if (argc != 3 && argc != 5){
		printf("wrong number of arguments.\n");
		exit(1);
	}
	if (argc == 5){
		if (strcmp(argv[3], "-p") == 0){
			port = atoi(argv[4]);	
		} else {
			printf("invalid argument\n");
			printf(usage);
			exit(1);
		}
	} 
	if (strcmp(argv[1], "-r") == 0) mode = RRQ;
	else if (strcmp(argv[1], "-w") == 0) mode = WRQ; 
	else {
		printf("invalid argument\n");
		printf(usage);
		exit(1);
	}
	char* filename = argv[2];
	init();
	
	bzero((char*)&serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port);

	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
		printf("%s: can't open datagram socket", progname);
		exit(1);
	}

	printf("tftp client socket created\n");

	bzero((char*)&cli_addr, sizeof(cli_addr));
	cli_addr.sin_family = AF_INET;
	
	cli_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	cli_addr.sin_port = htons(0);
	
	if (bind(sockfd, (struct sockaddr*)&cli_addr, sizeof(cli_addr)) < 0){
		printf("%s: can't bind local address\n", progname);
		exit(2);	
	}

	dg_cli(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr), filename, mode);

	close(sockfd);
	exit(0);
}

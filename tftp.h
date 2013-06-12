#ifndef TFTP_H
#define TFTP_H


#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>

#define SERV_UDP_PORT 60028

#define SERV_HOST_ADDR "127.0.0.1"


#define MAX_SIZE 516
#define MAX_DATA 512

#define RRQ	1
#define WRQ	2
#define DATA 	3
#define ACK 	4
#define ERROR	5

#define SENDING 4
#define ENDING	5

char* title = "Produced by Zewei Chu, Zhixin Xu, group 28:\n";

int timeout;
int recv_len;

#endif

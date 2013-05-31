#include "tftp.h"

handler(int signum){
	if (recv_len<0){
		timeout++;
		printf("timeout count #%d\n", timeout);
	}
}


init(){
	signal(SIGALRM, handler);
	siginterrupt(SIGALRM, 1);
	timeout = 0;
}

processError(char* mesg){
	unsigned short* p = (unsigned short*)(mesg+2);
	int errorcode = ntohs(*p);
	if (errorcode == 1) printf("File not found\n");
	else if (errorcode == 2) printf("Access violation\n");
	else if (errorcode == 6) printf("File already exists\n");
	exit(1);
}

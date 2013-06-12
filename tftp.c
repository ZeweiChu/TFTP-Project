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

processError(int errorcode){
	if (errorcode == 1) printf("file not found\n");
	else if (errorcode == 2) printf("access violation\n");
	else if (errorcode == 6) printf("file already exists\n");
	exit(1);
}

#include "tftp.h"

#include <>

#define MAX 				512
#define HALF_MAX 			255
#define MAX_TIMEOUTS		10


#define RRQ 				1
#define WRQ					2
#define DATA 				3
#define ACK  				4
#define ERROR 				5	




struct TFTP_RQ{
	signed short int opcode;
	char filename[HALF_MAX];
	char zero_0;
	char mode[HALF_MAX];
	char zero_1;
};

struct TFTP_DATA{
	signed short int opcode;
	signed short int block;
	char data[MAX];
};

struct TFTP_ACK{
	signed short int opcode;
	signed short int block;
};


